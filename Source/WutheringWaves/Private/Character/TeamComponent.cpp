// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TeamComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "UI/WuwaHUD.h"

// Sets default values for this component's properties
UTeamComponent::UTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTeamComponent::InitializeTeam()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	SpawnedTeam.Empty();

	// Spawn at the level's PlayerStart so moving it in the editor actually works.
	// Falls back to the fixed SpawnLocation if the level has no PlayerStart.
	FVector SpawnLoc = SpawnLocation;
	FRotator SpawnRot = FRotator::ZeroRotator;
	if (AGameModeBase* GameMode = World->GetAuthGameMode())
	{
		if (AActor* StartSpot = GameMode->FindPlayerStart(PC))
		{
			SpawnLoc = StartSpot->GetActorLocation();
			SpawnRot = StartSpot->GetActorRotation();
		}
	}

	// Spawn all the characters first
	for (int32 i = 0; i < TeamRoster.Num(); ++i)
	{
		if (!TeamRoster[i])
		{
			continue;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APlayableCharacter* NewChar = World->SpawnActor<APlayableCharacter>(
			TeamRoster[i], SpawnLoc, SpawnRot, Params);

		if (!NewChar)
		{
			continue;
		}

		SpawnedTeam.Add(NewChar);

		
		DeActivateCharacter(NewChar);
	}

	if (SpawnedTeam.Num() == 0)
	{
		return;
	}

	// Index 0 Character is first Activated
	ActiveIndex = 0;
	ActivateCharacter(SpawnedTeam[0]);
	PC->Possess(SpawnedTeam[0]);

	// Point the HUD at the starting character (no-op if the HUD isn't up yet - it seeds itself in BeginPlay)
	if (AWuwaHUD* HUD = Cast<AWuwaHUD>(PC->GetHUD()))
	{
		HUD->OnPlayerCharacterChanged(SpawnedTeam[0]);
	}
}

void UTeamComponent::SwitchCharacter(int32 Index)
{
	
	if (!SpawnedTeam.IsValidIndex(Index) || Index == ActiveIndex)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	APlayableCharacter* Outgoing = SpawnedTeam[ActiveIndex];
	APlayableCharacter* Incoming = SpawnedTeam[Index];

	if (!Incoming)
	{
		return;
	}

	// Save the Camera Position so that the changed charac can use it
	
	const FRotator SavedControlRotation = PC->GetControlRotation();

	
	if (Outgoing)
	{
		Incoming->SetActorLocationAndRotation(
			Outgoing->GetActorLocation(),
			Outgoing->GetActorRotation());

	
	}

	//Change Character, Deactivate + activate
	if (Outgoing)
	{
		DeActivateCharacter(Outgoing);
	}
	ActivateCharacter(Incoming);


	PC->Possess(Incoming);


	PC->SetControlRotation(SavedControlRotation);

	ActiveIndex = Index;

	// Rebind the HUD to the newly active character's ASC / level
	if (AWuwaHUD* HUD = Cast<AWuwaHUD>(PC->GetHUD()))
	{
		HUD->OnPlayerCharacterChanged(Incoming);
	}
}

void UTeamComponent::ActivateCharacter(APlayableCharacter* Char)
{
	if (!Char)
	{
		return;
	}

	Char->SetActorHiddenInGame(false);
	Char->SetActorEnableCollision(true);
	Char->SetActorTickEnabled(true);

	if (UCharacterMovementComponent* Movement = Char->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
}

void UTeamComponent::DeActivateCharacter(APlayableCharacter* Char)
{
	if (!Char)
	{
		return;
	}

	Char->SetActorHiddenInGame(true);
	Char->SetActorEnableCollision(false);
	Char->SetActorTickEnabled(false);

	if (UCharacterMovementComponent* Movement = Char->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
}
