// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayableCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Character/WuwaInputConfig.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Enemy/EnemyCharacter.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

#if ENABLE_DRAW_DEBUG
// Capture toggle for screenshots: console `wuwa.DebugTargeting 1` to draw the soft lock-on acquisition.
// Off in normal play; the draw calls compile out entirely in Shipping (ENABLE_DRAW_DEBUG == 0).
static TAutoConsoleVariable<int32> CVarDebugTargeting(
	TEXT("wuwa.DebugTargeting"),
	0,
	TEXT("Draw soft lock-on acquisition: aim cone (cyan), candidates (cyan = in cone, gray = out), ")
	TEXT("selected target (red), and the step-in / rotation blend arrows (orange). 0=off, 1=on."),
	ECVF_Cheat);
#endif

APlayableCharacter::APlayableCharacter()
{
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));

	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 400.0f;

	CameraComponent->SetupAttachment(SpringArmComponent);

	SpringArmComponent->bUsePawnControlRotation = true;
	CameraComponent->bUsePawnControlRotation = false;

	InputMappingContext = nullptr;
}

void APlayableCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bTargetAssistActive)
	{
		return;
	}

	TargetAssistElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(TargetAssistElapsed / TargetAssistBlendTime, 0.f, 1.f);
	const float S = FMath::SmoothStep(0.f, 1.f, Alpha);   // ease in / out so the turn isn't a hard snap

	const FQuat NewRot = FQuat::Slerp(TargetAssistStartRot.Quaternion(), TargetAssistGoalRot.Quaternion(), S);

	if (bTargetAssistStepIn)
	{
		const FVector NewLoc = FMath::Lerp(TargetAssistStartLoc, TargetAssistGoalLoc, S);
		SetActorLocationAndRotation(NewLoc, NewRot, true);   // sweep so we stop at walls instead of clipping
	}
	else
	{
		SetActorRotation(NewRot);
	}

#if ENABLE_DRAW_DEBUG
	if (CVarDebugTargeting.GetValueOnGameThread() > 0)
	{
		// one short-lived arrow per frame -> the blend leaves a fan tracing the turn (and step-in) toward the target
		DrawDebugDirectionalArrow(GetWorld(),
			GetActorLocation(), GetActorLocation() + GetActorForwardVector() * 120.f,
			30.f, FColor::Orange, false, 0.7f, 0, 2.f);
	}
#endif

	if (Alpha >= 1.f)
	{
		bTargetAssistActive = false;
		// hand rotation back to the movement component
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->bOrientRotationToMovement = bSavedOrientToMovement;
		}
	}
}

void APlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Null Exception (added to prevent a crash)
	if (!WuwaInputConfig)
	{
		return;
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(WuwaInputConfig->InputLook, ETriggerEvent::Triggered, this, &APlayableCharacter::Look);
		EnhancedInputComponent->BindAction(WuwaInputConfig->InputMove, ETriggerEvent::Triggered, this, &APlayableCharacter::Move);
		EnhancedInputComponent->BindAction(WuwaInputConfig->InputMouseWheel, ETriggerEvent::Triggered, this, &APlayableCharacter::MouseWheel);
		EnhancedInputComponent->BindAction(WuwaInputConfig->InputJump, ETriggerEvent::Started, this, &APlayableCharacter::Jump);
		EnhancedInputComponent->BindAction(WuwaInputConfig->InputJump, ETriggerEvent::Completed, this, &APlayableCharacter::StopJumping);

		// Sprint: hold the dodge key (Hold trigger) to run, release to walk
		if (WuwaInputConfig->InputSprint)
		{
			EnhancedInputComponent->BindAction(WuwaInputConfig->InputSprint, ETriggerEvent::Triggered, this, &APlayableCharacter::StartSprint);
			EnhancedInputComponent->BindAction(WuwaInputConfig->InputSprint, ETriggerEvent::Completed, this, &APlayableCharacter::StopSprint);
			EnhancedInputComponent->BindAction(WuwaInputConfig->InputSprint, ETriggerEvent::Canceled, this, &APlayableCharacter::StopSprint);
		}

		// Binding the GAS AbilitySkills - each entry picks its own trigger event
		// (attacks = Completed, dodge = Triggered so a Tap trigger fires it on tap while Hold sprints)
		for (FWuwaInput Action : WuwaInputConfig->InputTagList)
		{
			EnhancedInputComponent->BindAction(Action.InputAction, Action.TriggerEvent, this, &APlayableCharacter::InputTagUseAbility, Action.InputTag);
		}
	}
}

void APlayableCharacter::PossessedBy(AController* NewController)
{
	// Character Base : InitAbilityActorInfo + GiveAbilites 
	Super::PossessedBy(NewController);
	
	// no data asset -> HP/level/skills/etc. can't be applied. Warn loudly instead of silently using defaults,
	// since "one character has wrong HP and level" almost always means its CharacterData slot is empty.
	if (!CharacterData)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayableCharacter] %s has no CharacterData set - HP/level/skills will use defaults"),
			*GetName());
	}

	if (CharacterData)
	{
		// warn if the data asset is there but the HP field was left at (or below) zero - the character would
		// spawn dead / with a broken bar, and it looks identical to "HP not applied"
		if (CharacterData->MaxHp <= 0.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayableCharacter] %s CharacterData MaxHp is %.1f (<= 0)"),
				*GetName(), CharacterData->MaxHp);
		}

		// Initialize attributes only ONCE per character. Re-possessing on a team swap must NOT reset
		// HP / gauges back to full, otherwise damage taken before the swap is wiped on swap-back.
		if (!bAttributesInitialized)
		{
			bAttributesInitialized = true;

			InitializeAttributes(CharacterData->MaxHp);

			// 변주 게이지 (charged-swap circuit) starts empty, max from the data asset
			if (AttributeSet)
			{
				AttributeSet->InitMaxVariationEnergy(CharacterData->MaxVariationEnergy);
				AttributeSet->InitVariationEnergy(0.f);
			}
		}

		// start at walk speed (sprint switches to RunSpeed while the dodge key is held)
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->MaxWalkSpeed = CharacterData->WalkSpeed;
		}
	}

	AddInputMapping();
}

int32 APlayableCharacter::GetCharacterLevel() const
{
	return CharacterData ? CharacterData->Level : 1;
}

UAnimMontage* APlayableCharacter::GetHitReactMontage() const
{
	// data-driven: pull the flinch montage from this character's data asset
	return CharacterData ? CharacterData->HitReactMontage : nullptr;
}

void APlayableCharacter::PlayDodgeSlowMo()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// slow the whole world down
	UGameplayStatics::SetGlobalTimeDilation(World, DodgeSlowMoScale);

	// optionally counter it on the player so it acts near-normal while the world crawls (reaction advantage)
	CustomTimeDilation = bDodgeSlowMoKeepPlayerFast ? (1.f / DodgeSlowMoScale) : 1.f;

	// world timers run on DILATED time, so scale the duration to fire after DodgeSlowMoDuration *real* seconds
	World->GetTimerManager().SetTimer(SlowMoTimerHandle, this, &APlayableCharacter::EndDodgeSlowMo,
		FMath::Max(DodgeSlowMoDuration * DodgeSlowMoScale, 0.001f), false);
}

void APlayableCharacter::EndDodgeSlowMo()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	}
	CustomTimeDilation = 1.f;
}

const TArray<TObjectPtr<USoundBase>>& APlayableCharacter::GetHitVoiceLines() const
{
	// data-driven: pull the hit voice lines from this character's data asset
	static const TArray<TObjectPtr<USoundBase>> Empty;
	return CharacterData ? CharacterData->HitVoiceLines : Empty;
}

void APlayableCharacter::UnPossessed()
{
	RemoveInputMapping();

	Super::UnPossessed();
}

void APlayableCharacter::AddInputMapping()
{
	if (!InputMappingContext)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}
}

void APlayableCharacter::RemoveInputMapping()
{
	if (!InputMappingContext)
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(InputMappingContext);
		}
	}
}

void APlayableCharacter::InputTagUseAbility(FGameplayTag InputTag)
{
	if (!AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag)))
	{
		for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			if (!Spec.IsActive() || !Spec.Ability) continue;
			if (!Spec.Ability->AbilityTags.HasTag(InputTag)) continue;

			UGameplayAbility* Instance = Spec.GetPrimaryInstance();
			if (Instance)
			{
				Instance->InputPressed(Spec.Handle, AbilitySystemComponent->AbilityActorInfo.Get(), Spec.ActivationInfo);
			}
			break;
		}
	}
}

void APlayableCharacter::Move(const FInputActionValue& value)
{
	if (!Controller) return;

	FVector2D InputVal = value.Get<FVector2D>();


	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);


	const FVector CameraForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector CameraRight   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);


	if (!FMath::IsNearlyZero(InputVal.X))
	{
		AddMovementInput(CameraForward, InputVal.X);
	}

	if (!FMath::IsNearlyZero(InputVal.Y))
	{
		AddMovementInput(CameraRight, InputVal.Y);
	}
}

void APlayableCharacter::Look(const FInputActionValue& value)
{
	if (!Controller) return;

	FVector2D Inputval = value.Get<FVector2D>();

	if (!Inputval.IsNearlyZero())
	{
		AddControllerYawInput(Inputval.X);
		AddControllerPitchInput(Inputval.Y);
	}
}

void APlayableCharacter::StartSprint()
{
	if (CharacterData)
	{
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->MaxWalkSpeed = CharacterData->RunSpeed;
		}
	}
}

void APlayableCharacter::StopSprint()
{
	if (CharacterData)
	{
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->MaxWalkSpeed = CharacterData->WalkSpeed;
		}
	}
}

void APlayableCharacter::MouseWheel(const FInputActionValue& value)
{
	float WheelValue = value.Get<float>();

	if (WheelValue > 0)
	{
		if (SpringArmComponent->TargetArmLength > 50.0f)
		{
			SpringArmComponent->TargetArmLength -= 5.f;
		}
	}

	if (WheelValue < 0)
	{
		if (SpringArmComponent->TargetArmLength < 250)
		{
			SpringArmComponent->TargetArmLength += 5.f;
		}
	}
}

//============================================================================
// Auto-target ("soft lock")
//============================================================================

AEnemyCharacter* APlayableCharacter::FindNearestEnemy() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyCharacter::StaticClass(), Enemies);

	AEnemyCharacter* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	const FVector From = GetActorLocation();

	for (AActor* Actor : Enemies)
	{
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Actor);
		if (!Enemy || Enemy->IsDead())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(From, Enemy->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Enemy;
		}
	}
	return Best;
}

AEnemyCharacter* APlayableCharacter::AcquireTargetEnemy() const
{
	UWorld* World = GetWorld();
	if (!World || !CharacterData)
	{
		return nullptr;
	}

	const FTargetAssistData& Cfg = CharacterData->TargetAssist;

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyCharacter::StaticClass(), Enemies);

	const FVector Origin = GetActorLocation();

	// aim reference = where the camera looks (flattened to the ground plane)
	FVector AimDir = CameraComponent ? CameraComponent->GetForwardVector() : GetActorForwardVector();
	AimDir.Z = 0.f;
	AimDir = AimDir.GetSafeNormal();

	const float CosHalfCone = FMath::Cos(FMath::DegreesToRadians(Cfg.ConeHalfAngleDeg));
	const float RangeSq = Cfg.Range * Cfg.Range;

#if ENABLE_DRAW_DEBUG
	const bool bDbgTargeting = CVarDebugTargeting.GetValueOnGameThread() > 0;
#endif

	AEnemyCharacter* Best = nullptr;
	bool bBestInCone = false;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AActor* Actor : Enemies)
	{
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Actor);
		if (!Enemy || Enemy->IsDead())
		{
			continue;
		}

		FVector ToEnemy = Enemy->GetActorLocation() - Origin;
		ToEnemy.Z = 0.f;
		const float DistSq = ToEnemy.SizeSquared();
		if (DistSq > RangeSq)
		{
			continue;   // out of range
		}

		const float Dot = FVector::DotProduct(ToEnemy.GetSafeNormal(), AimDir);
		const bool bInCone = Dot >= CosHalfCone;

#if ENABLE_DRAW_DEBUG
		if (bDbgTargeting)
		{
			// cyan = eligible (inside cone), gray = considered but outside the cone
			DrawDebugSphere(World, Enemy->GetActorLocation(), 45.f, 12,
				bInCone ? FColor::Cyan : FColor(90, 90, 90), false, 2.5f);
		}
#endif

		// prefer targets inside the aim cone; within the same class, prefer the nearest.
		const bool bBetter = (bInCone && !bBestInCone) ||
			(bInCone == bBestInCone && DistSq < BestDistSq);
		if (bBetter)
		{
			Best = Enemy;
			bBestInCone = bInCone;
			BestDistSq = DistSq;
		}
	}

#if ENABLE_DRAW_DEBUG
	if (bDbgTargeting)
	{
		// the camera-facing aim cone the acquisition searched, plus the winner (red)
		const float ConeRad = FMath::DegreesToRadians(Cfg.ConeHalfAngleDeg);
		DrawDebugCone(World, Origin, AimDir, Cfg.Range, ConeRad, ConeRad, 24, FColor::Cyan, false, 2.5f, 0, 1.5f);
		if (Best)
		{
			DrawDebugSphere(World, Best->GetActorLocation(), 55.f, 16, FColor::Red, false, 2.5f, 0, 3.f);
			DrawDebugLine(World, Origin, Best->GetActorLocation(), FColor::Red, false, 2.5f, 0, 3.f);
		}
	}
#endif

	return Best;
}

AEnemyCharacter* APlayableCharacter::FaceTargetForAttack()
{
	if (!CharacterData || !CharacterData->TargetAssist.bEnabled)
	{
		return nullptr;
	}

	AEnemyCharacter* Target = AcquireTargetEnemy();
	if (!Target)
	{
		return nullptr;
	}

	const FTargetAssistData& Cfg = CharacterData->TargetAssist;
	const FVector Origin = GetActorLocation();

	FVector ToEnemy = Target->GetActorLocation() - Origin;
	ToEnemy.Z = 0.f;
	const float Dist = ToEnemy.Size();
	if (Dist <= KINDA_SMALL_NUMBER)
	{
		return Target;
	}
	const FVector Dir = ToEnemy / Dist;

	// goal facing: yaw toward the enemy (keep us upright)
	TargetAssistStartRot = GetActorRotation();
	TargetAssistGoalRot = FRotator(0.f, Dir.Rotation().Yaw, 0.f);

	// goal location: slide in to StopDistance, capped by MaxStepDistance, only if asked and we're too far
	TargetAssistStartLoc = Origin;
	TargetAssistGoalLoc = Origin;
	bTargetAssistStepIn = false;
	if (Cfg.bStepIn && Dist > Cfg.StopDistance)
	{
		const float Step = FMath::Min(Dist - Cfg.StopDistance, Cfg.MaxStepDistance);
		TargetAssistGoalLoc = Origin + Dir * Step;
		bTargetAssistStepIn = true;
	}

	// take rotation control for the blend so orient-to-movement doesn't fight it; restored when it ends.
	// only capture the saved value if a blend isn't already running (else we'd save our own false).
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		if (!bTargetAssistActive)
		{
			bSavedOrientToMovement = Move->bOrientRotationToMovement;
		}
		Move->bOrientRotationToMovement = false;
	}

	TargetAssistElapsed = 0.f;
	TargetAssistBlendTime = FMath::Max(Cfg.BlendTime, 0.01f);
	bTargetAssistActive = true;

	return Target;
}
