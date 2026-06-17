#include "Character/BaseCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Character/WuwaInputConfig.h"
#include "GameFramework/SpringArmComponent.h"


ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->TargetArmLength = 300.0f;
	SpringArmComponent->bUsePawnControlRotation = true;
	
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;
	
	//Initialize
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
	
	if (!FMath::IsNearlyZero(InputVal.X))
	{
		AddMovementInput(GetActorForwardVector(),InputVal.X);
	}
	
	if (!FMath::IsNearlyZero(InputVal.Y))
	{
		AddMovementInput(GetActorRightVector(),InputVal.Y);
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

