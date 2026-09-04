// Fill out your copyright notice in the Description page of Project Settings.



#include "Enemy/EnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DataAsset/EnemyDataAsset.h"
#include "Enemy/WuwaEnemyController.h"
#include "GameAbilities/GA_EnemyAttack.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "BrainComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "MotionWarpingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UI/WuwaHUD.h"

AEnemyCharacter::AEnemyCharacter()
{
	// wire the possess plumbing in C++ so a BP setup mistake can't break it
	// (last time "abilities not granted" was because possess never happened, this prevents that)
	AIControllerClass = AWuwaEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Let the player's camera boom pass THROUGH enemies. Otherwise getting close (especially to a large
	// enemy) makes the spring arm's Camera-channel probe hit the enemy capsule and yank the camera inside
	// the player for a frame. Done in BeginPlay so it sticks even if the BP uses a collision preset.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	
	if (EnemyDataAsset)
	{
		InitializeAttributes(EnemyDataAsset->MaxHp);

		// groggy starts full and drains as the boss gets hit; at 0 it enters the groggy state
		if (AttributeSet)
		{
			AttributeSet->InitMaxGroggy(EnemyDataAsset->MaxGroggy);
			AttributeSet->InitGroggy(EnemyDataAsset->MaxGroggy);
		}

		// move speed also comes from the data asset (each monster can have its own speed)
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->MaxWalkSpeed = EnemyDataAsset->MoveSpeed;
		}
	}

	// enemy grants abilities here directly instead of relying on AIController possess
	// (bAbilitiesGranted guard means a later PossessedBy won't grant them twice)
	GiveAbilites();

	// bosses show the top-screen health bar for as long as they're alive
	if (EnemyDataAsset && EnemyDataAsset->bIsBoss)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AWuwaHUD* HUD = Cast<AWuwaHUD>(PC->GetHUD()))
			{
				HUD->ShowBossBar(this);
			}
		}

		// start the boss battle music (2D, loops via the asset). Kept as a component so death can fade it.
		if (EnemyDataAsset->BattleMusic)
		{
			BattleMusicComp = UGameplayStatics::SpawnSound2D(this, EnemyDataAsset->BattleMusic);
		}
	}

	// spawn intro: play montage + voice and hold the AI until it finishes (bosses or regular enemies)
	if (EnemyDataAsset && EnemyDataAsset->IntroMontage)
	{
		PlayIntro();
	}
}

void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// during an attack wind-up, keep turning toward the player so the swing / lunge aims at them instead of
	// committing to wherever the enemy happened to face when the montage started. GA_EnemyAttack turns this
	// off at the strike, so the actual hit is committed (and thus dodgeable).
	if (bAttackTracking && !bIsGroggy)
	{
		FacePlayerYaw(DeltaTime);
	}

	// groggy only refills WHILE staggered (not during normal combat, and not during the get-up phase).
	// GroggyRecoverTime = seconds for the full 0 -> max recovery.
	if (!bIsGroggy || bIsRecovering || !AttributeSet || !EnemyDataAsset || EnemyDataAsset->GroggyRecoverTime <= 0.f)
	{
		return;
	}

	const float MaxG = AttributeSet->GetMaxGroggy();
	const float CurG = AttributeSet->GetGroggy();
	const float Rate = MaxG / EnemyDataAsset->GroggyRecoverTime; // per second
	const float NewG = FMath::Min(CurG + Rate * DeltaTime, MaxG);
	AttributeSet->SetGroggy(NewG);

	if (NewG >= MaxG)
	{
		ExitGroggy(); // fully recovered
	}
}

bool AEnemyCharacter::TryDodge()
{
	// can't dodge while dead / staggered / already mid-dodge, or if no dodge is configured
	if (bIsDead || bIsGroggy || bIsDodging || !EnemyDataAsset)
	{
		return false;
	}
	if (!EnemyDataAsset->DodgeMontage || EnemyDataAsset->DodgeChance <= 0.f)
	{
		return false;
	}

	// roll the chance (FRand is [0,1)) - miss -> the hit connects normally
	if (FMath::FRand() > EnemyDataAsset->DodgeChance)
	{
		return false;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* Anim = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!Anim)
	{
		return false;
	}

	// play the backstep. If it fails to play, let the hit connect (don't swallow damage for nothing).
	const float PlayLen = Anim->Montage_Play(EnemyDataAsset->DodgeMontage);
	if (PlayLen <= 0.f)
	{
		return false;
	}

	bIsDodging = true;

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyCharacter::OnDodgeMontageEnded);
	Anim->Montage_SetEndDelegate(EndDelegate, EnemyDataAsset->DodgeMontage);

	return true;
}

void AEnemyCharacter::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsDodging = false;
}

void AEnemyCharacter::PlayIntro()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* Anim = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!Anim || !EnemyDataAsset || !EnemyDataAsset->IntroMontage)
	{
		return;
	}

	// turn to face the player so the intro is aimed at them (not at wherever the enemy was placed)
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
		ToPlayer.Z = 0.f;
		if (!ToPlayer.IsNearlyZero())
		{
			FRotator Face = ToPlayer.Rotation();
			Face.Pitch = 0.f;
			Face.Roll = 0.f;
			SetActorRotation(Face);
		}
	}

	const float PlayLen = Anim->Montage_Play(EnemyDataAsset->IntroMontage);
	if (PlayLen <= 0.f)
	{
		return;   // couldn't play -> don't gate combat on an intro that never runs
	}

	bIntroPlaying = true;

	// pause the AI now if we're already possessed (placed enemies). If we're possessed later, the
	// controller's OnPossess pauses via IsPlayingIntro() instead - so combat is held either way.
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (AI->GetBrainComponent())
		{
			AI->GetBrainComponent()->PauseLogic(TEXT("Intro"));
		}
	}

	// intro voice line
	if (EnemyDataAsset->IntroVoice)
	{
		UGameplayStatics::SpawnSoundAttached(EnemyDataAsset->IntroVoice, MeshComp);
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AEnemyCharacter::OnIntroMontageEnded);
	Anim->Montage_SetEndDelegate(EndDelegate, EnemyDataAsset->IntroMontage);
}

void AEnemyCharacter::OnIntroMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIntroPlaying = false;

	// intro done -> let the AI start fighting
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (AI->GetBrainComponent())
		{
			AI->GetBrainComponent()->ResumeLogic(TEXT("Intro"));
		}
	}
}

void AEnemyCharacter::PlayHitReact()
{
	// never flinch while staggered / getting up - it would cut the groggy loop or the get-up montage
	if (bIsGroggy || bIsRecovering)
	{
		return;
	}

	// only flinch some of the time - reacting to every hit looks spammy and causes stunlock.
	// 70% of hits the enemy "poises" through (keeps its attack), 30% it flinches.
	if (EnemyDataAsset && FMath::FRand() > EnemyDataAsset->HitReactChance)
	{
		return;
	}

	Super::PlayHitReact();
}

void AEnemyCharacter::SetAttackTracking(bool bEnable)
{
	if (bEnable == bAttackTracking)
	{
		return;
	}
	bAttackTracking = bEnable;

	// While aiming, turn OFF orient-to-movement. Otherwise the movement component re-faces the enemy toward
	// its velocity every frame and cancels our FacePlayerYaw - which is why the attack "just kept going straight".
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		if (bEnable)
		{
			bSavedOrientToMovement = Move->bOrientRotationToMovement;
			Move->bOrientRotationToMovement = false;
		}
		else
		{
			Move->bOrientRotationToMovement = bSavedOrientToMovement;
		}
	}
}

void AEnemyCharacter::FacePlayerYaw(float DeltaTime)
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.f;
	if (ToPlayer.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRot = GetActorRotation();
	FRotator GoalRot = ToPlayer.Rotation();
	GoalRot.Pitch = CurrentRot.Pitch;   // yaw only - keep the enemy upright
	GoalRot.Roll  = CurrentRot.Roll;

	// Motion Warping: keep "AttackTarget" pointed at the player each frame, so a root-motion attack (with a
	// Motion Warping notify on its montage) HOMES toward the player during the wind-up instead of going straight.
	if (MotionWarping)
	{
		MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(
			FName("AttackTarget"), Player->GetActorLocation(), FRotator(0.f, GoalRot.Yaw, 0.f));
	}

	// non-root-motion attacks: also turn manually (harmless when Motion Warping is driving the rotation)
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, GoalRot.Yaw)) > AttackTrackDeadzoneDeg)
	{
		const FRotator NewRot = (AttackTurnSpeed > 0.f)
			? FMath::RInterpTo(CurrentRot, GoalRot, DeltaTime, AttackTurnSpeed)
			: GoalRot;
		SetActorRotation(NewRot);
	}
}

void AEnemyCharacter::ApplyGroggyDamage(float Amount)
{
	// already staggered/recovering or dead: ignore further groggy hits
	if (bIsGroggy || bIsDead || !AttributeSet || Amount <= 0.f)
	{
		return;
	}

	const float NewG = FMath::Max(AttributeSet->GetGroggy() - Amount, 0.f);
	AttributeSet->SetGroggy(NewG);

	if (NewG <= 0.f)
	{
		EnterGroggy();
	}
}

void AEnemyCharacter::EnterGroggy()
{
	if (bIsGroggy)
	{
		return;
	}
	bIsGroggy = true;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(StateTags::Enemy_State_Groggy);
	}

	// stop attacking / AI logic while staggered
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (AI->GetBrainComponent())
		{
			AI->GetBrainComponent()->StopLogic(TEXT("Groggy"));
		}
	}

	// play the stagger animation and force it to loop (regardless of the montage's section setup),
	// so it keeps playing for the whole stagger no matter how long GroggyRecoverTime is
	if (EnemyDataAsset && EnemyDataAsset->GroggyMontage)
	{
		UAnimMontage* StaggerMontage = EnemyDataAsset->GroggyMontage;
		PlayAnimMontage(StaggerMontage);

		if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			// loop ONLY the "staying down" section so the collapse intro plays once.
			// if that named section doesn't exist, fall back to looping from the first section.
			FName LoopSection = EnemyDataAsset->GroggyLoopSection;
			if (StaggerMontage->GetSectionIndex(LoopSection) == INDEX_NONE)
			{
				LoopSection = StaggerMontage->GetNumSections() > 0 ? StaggerMontage->GetSectionName(0) : NAME_None;
			}

			if (!LoopSection.IsNone())
			{
				// point the section at itself -> loops until the get-up montage replaces it
				Anim->Montage_SetNextSection(LoopSection, LoopSection, StaggerMontage);
			}
		}
	}

	// from here Tick gradually refills groggy 0 -> full; when full, ExitGroggy runs.
	UE_LOG(LogTemp, Warning, TEXT("[Groggy] %s entered groggy"), *GetName());
}

void AEnemyCharacter::ExitGroggy()
{
	if (!bIsGroggy || bIsRecovering)
	{
		return;
	}
	bIsRecovering = true;

	// stagger/vulnerable window is over: drop the groggy tag (1.5x ends, bar turns back to normal color)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(StateTags::Enemy_State_Groggy);
	}

	// play the get-up montage (replaces the looping stagger); AI stays paused until it ends
	if (EnemyDataAsset && EnemyDataAsset->GroggyRecoverMontage)
	{
		PlayAnimMontage(EnemyDataAsset->GroggyRecoverMontage);

		if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AEnemyCharacter::OnGroggyRecoverMontageEnded);
			Anim->Montage_SetEndDelegate(EndDelegate, EnemyDataAsset->GroggyRecoverMontage);
		}
		else
		{
			FinishGroggyRecovery();
		}
	}
	else
	{
		// no get-up montage: stop the stagger loop and recover right away
		if (EnemyDataAsset && EnemyDataAsset->GroggyMontage)
		{
			StopAnimMontage(EnemyDataAsset->GroggyMontage);
		}
		FinishGroggyRecovery();
	}

	UE_LOG(LogTemp, Warning, TEXT("[Groggy] %s recovering (get-up)"), *GetName());
}

void AEnemyCharacter::OnGroggyRecoverMontageEnded(UAnimMontage* /*Montage*/, bool /*bInterrupted*/)
{
	FinishGroggyRecovery();
}

void AEnemyCharacter::FinishGroggyRecovery()
{
	if (!bIsGroggy)
	{
		return;
	}
	bIsGroggy = false;
	bIsRecovering = false;

	// died mid-recovery: don't touch AI
	if (bIsDead)
	{
		return;
	}

	// resume AI logic now that the get-up animation is done
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (AI->GetBrainComponent())
		{
			AI->GetBrainComponent()->RestartLogic();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Groggy] %s recovered"), *GetName());
}

int32 AEnemyCharacter::GetCurrentPhase() const
{
	if (!AttributeSet || !EnemyDataAsset)
	{
		return 1;
	}

	const float MaxHpValue = AttributeSet->GetMaxHp();
	if (MaxHpValue <= 0.f)
	{
		return 1;
	}

	const float Ratio = AttributeSet->GetHp() / MaxHpValue;

	// PhaseHpRatios is descending phase goes up by the number of thresholds passed
	int32 Phase = 1;
	for (const float Threshold : EnemyDataAsset->PhaseHpRatios)
	{
		if (Ratio <= Threshold)
		{
			Phase++;
		}
	}
	return Phase;
}

void AEnemyCharacter::PerformAttack()
{
	if (!AbilitySystemComponent || !EnemyDataAsset)
	{
		return;
	}

	const int32 Phase = GetCurrentPhase();

	// step 1: gather attack type tags usable in the current phase (MinPhase <= Phase)
	// skip any type that has no montages
	TArray<FGameplayTag> EligibleTags;
	for (const TPair<FGameplayTag, FEnemySkillData>& Skill : EnemyDataAsset->Skills)
	{
		if (Skill.Value.MinPhase <= Phase && Skill.Value.Montages.Num() > 0)
		{
			EligibleTags.Add(Skill.Key);
		}
	}

	if (EligibleTags.Num() == 0)
	{
		return;
	}

	// pick one tag at random (can extend to distance/weight based later)
	CurrentSkillTag = EligibleTags[FMath::RandRange(0, EligibleTags.Num() - 1)];

	// step 2: pick one montage at random from that tag's montage list
	const FEnemySkillData& Skill = EnemyDataAsset->Skills[CurrentSkillTag];
	CurrentMontage = Skill.Montages[FMath::RandRange(0, Skill.Montages.Num() - 1)];

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] Phase %d -> skill %s"), Phase, *CurrentSkillTag.ToString());

	// find GA_EnemyAttack (or its BP child) by class hierarchy (IsA) and activate it
	// TryActivateAbilityByClass matches the exact class only so it fails for a BP child
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->IsA(UGA_EnemyAttack::StaticClass()))
		{
			AbilitySystemComponent->TryActivateAbility(Spec.Handle);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] GA_EnemyAttack not granted"));
}

void AEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

	// take the boss bar down with the boss
	if (EnemyDataAsset && EnemyDataAsset->bIsBoss)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AWuwaHUD* HUD = Cast<AWuwaHUD>(PC->GetHUD()))
			{
				HUD->HideBossBar();
			}
		}

		// fade the battle music out with the boss. The audio component lives independently of this actor,
		// so it keeps fading even after Destroy() below.
		if (BattleMusicComp)
		{
			BattleMusicComp->FadeOut(2.f, 0.f);
			BattleMusicComp = nullptr;
		}
	}

	Destroy();
}
