#include "Character/BaseCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Character/WuwaInputConfig.h"
#include "GameFramework/SpringArmComponent.h"

#include "GameFramework/CharacterMovementComponent.h" 

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
