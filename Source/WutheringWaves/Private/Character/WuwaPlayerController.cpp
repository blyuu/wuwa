// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/WuwaPlayerController.h"

#include "Character/TeamComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AWuwaPlayerController::AWuwaPlayerController()
{
	
	TeamComponent = CreateDefaultSubobject<UTeamComponent>(TEXT("TeamComponent"));
}

void AWuwaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (SwitchMappingContext)
		{
			Subsystem->AddMappingContext(SwitchMappingContext, 1);
		}
	}


	if (TeamComponent)
	{
		TeamComponent->InitializeTeam();
	}
}

void AWuwaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		
		for (int32 i = 0; i < SwitchActions.Num(); ++i)
		{
			if (SwitchActions[i])
			{
				EnhancedInputComponent->BindAction(
					SwitchActions[i], ETriggerEvent::Started, this, &AWuwaPlayerController::HandleSwitch, i);
			}
		}
	}
}

void AWuwaPlayerController::HandleSwitch(int32 Index)
{
	if (TeamComponent)
	{
		TeamComponent->SwitchCharacter(Index);
	}
}
