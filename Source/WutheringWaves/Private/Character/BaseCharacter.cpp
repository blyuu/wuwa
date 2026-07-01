#include "Character/BaseCharacter.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/WuwaInputConfig.h"
#include "Animation/AnimationAsset.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "GameFramework/SpringArmComponent.h"


ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
    
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 250.0f;
	
	CameraComponent->SetupAttachment(SpringArmComponent);

	SpringArmComponent->bUsePawnControlRotation = true; 
	CameraComponent->bUsePawnControlRotation = false;
	
	bUseControllerRotationYaw = false; 
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
    

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); 
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	AttributeSet = CreateDefaultSubobject<UWuWa_AttributeSetBase>(TEXT("AttributeSystem"));
	
	InputMappingContext = nullptr;
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	//IMC Mapping
	
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext,0);
			}
		}
	}
}


void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(WuwaInputConfig->InputLook, ETriggerEvent::Triggered, this, &ABaseCharacter::Look);
		EnhancedInputComponent->BindAction(WuwaInputConfig->InputMove, ETriggerEvent::Triggered, this, &ABaseCharacter::Move);
		EnhancedInputComponent->BindAction(WuwaInputConfig->InputMouseWheel, ETriggerEvent::Triggered, this, &ABaseCharacter::MouseWheel);
		EnhancedInputComponent->BindAction(WuwaInputConfig->InputJump, ETriggerEvent::Started, this, &ABaseCharacter::Jump);
		EnhancedInputComponent->BindAction(WuwaInputConfig->InputJump, ETriggerEvent::Completed, this, &ABaseCharacter::StopJumping);
		
		
		// Ability Skills과 관련된 키와 그걸 실행하는 함수 바인딩
		if (WuwaInputConfig)
		{
			for (FWuwaInput Action : WuwaInputConfig->InputTagList)
			{
				EnhancedInputComponent->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &ABaseCharacter::InputTagUseAbility, Action.InputTag);
			}
		}
	}
}

UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	
	
	//현재 캐릭터에게 Give Ability를 일괄적으로 제공하는 함수
	GiveAbilites();
}


void ABaseCharacter::InputTagUseAbility(FGameplayTag InputTag)
{
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(InputTag));
}

void ABaseCharacter::GiveAbilites()
{
	if (Abilites.IsEmpty())
	{
		return;
	}
	
	//BP에서 등록한 GameAbilities를 일괄적으로 해당 캐릭터에게 Give Ability
	for (int i = 0; i<Abilites.Num();i++)
	{
		FGameplayAbilitySpec AbilitySpec(Abilites[i], 1, -1);
		FGameplayAbilitySpecHandle Checker = AbilitySystemComponent->GiveAbility(AbilitySpec);
		
		if (Checker.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("등록 성공!"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("등록 실패!"));
		}
	}
}

void ABaseCharacter::Move(const FInputActionValue& value)
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

void ABaseCharacter::Look(const FInputActionValue& value)
{
	if (!Controller) return;
    
	FVector2D Inputval = value.Get<FVector2D>();
    
	if (!Inputval.IsNearlyZero())
	{
		AddControllerYawInput(Inputval.X);
		AddControllerPitchInput(Inputval.Y);
	}
}

void ABaseCharacter::MouseWheel(const FInputActionValue& value)
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

