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

	// SetByCaller (Data.Damage) GE used for the charged-swap 5% fixed damage
	UPROPERTY(EditDefaultsOnly, Category = "Team|Charged Swap")
	TSubclassOf<class UGameplayEffect> ChargedSwapDamageEffect;

	// fraction of the target's MaxHp dealt by a charged swap (0.05 = 5%)
	UPROPERTY(EditDefaultsOnly, Category = "Team|Charged Swap")
	float ChargedSwapDamagePercent = 0.05f;

	// how long the outgoing character stays and attacks (outro) before the charged swap actually happens
	UPROPERTY(EditDefaultsOnly, Category = "Team|Charged Swap")
	float ChargedSwapOutroTime = 1.0f;


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

	// exposed for the team portrait HUD
	const TArray<TObjectPtr<APlayableCharacter>>& GetTeam() const { return SpawnedTeam; }
	int32 GetActiveIndex() const { return ActiveIndex; }


private:
	void ActivateCharacter(APlayableCharacter* Char);

	void DeActivateCharacter(APlayableCharacter* Char);

	// the actual character swap (deactivate outgoing, activate + possess incoming, update HUD)
	void DoSwap(int32 Index);

	// charged swap (variation gauge full)
	bool IsVariationFull(APlayableCharacter* Char) const;
	void PerformChargedSwapEffect(APlayableCharacter* Instigator);
	void FinishChargedSwap();   // fired after the outro delay: swap + trigger the incoming's GA_Intro
	class AEnemyCharacter* FindNearestEnemy(const FVector& From) const;

	FTimerHandle ChargedSwapTimer;
	int32 PendingSwapIndex = INDEX_NONE;

};
