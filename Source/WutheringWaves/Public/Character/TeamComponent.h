// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/PlayableCharacter.h"
#include "TeamComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WUTHERINGWAVES_API UTeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTeamComponent();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Team")
	TArray<TSubclassOf<APlayableCharacter>> TeamRoster;


	//Spawn the Character first, something like object pooling
	UPROPERTY(EditDefaultsOnly, Category= "Team")
	FVector SpawnLocation;

private:
	UPROPERTY()
	TArray<TObjectPtr<APlayableCharacter>> SpawnedTeam;


	// Current Playing Character
	int32 ActiveIndex = 0;

public:

	void InitializeTeam();

	void SwitchCharacter(int32 Index);


private:
	void ActivateCharacter(APlayableCharacter* Char);

	void DeActivateCharacter(APlayableCharacter* Char);

};
