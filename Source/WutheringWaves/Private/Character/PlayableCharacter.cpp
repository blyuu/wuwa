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

APlayableCharacter::APlayableCharacter()
{
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));

	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 300.0f;

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
	
	if (CharacterData)
	{
		InitializeAttributes(CharacterData->MaxHp);

		// 변주 게이지 (charged-swap circuit) starts empty, max from the data asset
		if (AttributeSet)
		{
			AttributeSet->InitMaxVariationEnergy(CharacterData->MaxVariationEnergy);
			AttributeSet->InitVariationEnergy(0.f);
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
