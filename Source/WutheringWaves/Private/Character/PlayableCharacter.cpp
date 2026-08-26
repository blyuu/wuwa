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

APlayableCharacter::APlayableCharacter()
{
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));

	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 250.0f;

	CameraComponent->SetupAttachment(SpringArmComponent);

	SpringArmComponent->bUsePawnControlRotation = true;
	CameraComponent->bUsePawnControlRotation = false;

	InputMappingContext = nullptr;
}

void APlayableCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Null Exception (크래시 방지용으로 추가해봄)
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

		// Binding the GAS AbilitySkills
		for (FWuwaInput Action : WuwaInputConfig->InputTagList)
		{
			EnhancedInputComponent->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &APlayableCharacter::InputTagUseAbility, Action.InputTag);
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
	}
	
	AddInputMapping();
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
