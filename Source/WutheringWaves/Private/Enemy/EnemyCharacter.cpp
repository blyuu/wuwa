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
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "UI/WuwaHUD.h"

AEnemyCharacter::AEnemyCharacter()
{
	// wire the possess plumbing in C++ so a BP setup mistake can't break it
	// (last time "abilities not granted" was because possess never happened, this prevents that)
	AIControllerClass = AWuwaEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

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
	}
}

void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
	}

	Destroy();
}
