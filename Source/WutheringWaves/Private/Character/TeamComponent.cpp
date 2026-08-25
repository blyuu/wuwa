// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TeamComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UTeamComponent::UTeamComponent()
{
	// 이 컴포넌트는 매 프레임 Tick 할 필요가 없음 (교체는 이벤트성 동작)
	PrimaryComponentTick.bCanEverTick = false;
}

void UTeamComponent::InitializeTeam()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 이 컴포넌트는 PlayerController 위에 붙어있다고 가정
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	SpawnedTeam.Empty();

	// 로스터에 등록된 캐릭터 전원을 미리 스폰
	for (int32 i = 0; i < TeamRoster.Num(); ++i)
	{
		if (!TeamRoster[i])
		{
			continue;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABaseCharacter* NewChar = World->SpawnActor<ABaseCharacter>(
			TeamRoster[i], SpawnLocation, FRotator::ZeroRotator, Params);

		if (!NewChar)
		{
			continue;
		}

		SpawnedTeam.Add(NewChar);

		// 일단 전원 재워둔다 (숨김 + 충돌/이동 Off)
		DeActivateCharacter(NewChar);
	}

	if (SpawnedTeam.Num() == 0)
	{
		return;
	}

	// 0번 캐릭터만 활성화 + possess
	ActiveIndex = 0;
	ActivateCharacter(SpawnedTeam[0]);
	PC->Possess(SpawnedTeam[0]);
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

	ABaseCharacter* Outgoing = SpawnedTeam[ActiveIndex];
	ABaseCharacter* Incoming = SpawnedTeam[Index];

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

	 
}

void UTeamComponent::ActivateCharacter(ABaseCharacter* Char)
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

void UTeamComponent::DeActivateCharacter(ABaseCharacter* Char)
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
