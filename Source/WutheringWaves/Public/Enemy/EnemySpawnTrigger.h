// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawnTrigger.generated.h"

// A box the player walks into. On the first entry it tells its linked SpawnBox to spawn, then stops firing.
// Place it at the arena entrance and link it to a AEnemySpawnBox placed where the enemy should appear.
UCLASS()
class WUTHERINGWAVES_API AEnemySpawnTrigger : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawnTrigger();

	// the spawn point this trigger activates. In the level, drag the AEnemySpawnBox actor into this slot.
	UPROPERTY(EditInstanceOnly, Category = "Spawn")
	TObjectPtr<class AEnemySpawnBox> SpawnBox;

protected:
	virtual void BeginPlay() override;

	// player entered the box -> ask the linked SpawnBox to spawn, once
	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// the trigger volume (also the root). Only overlaps pawns.
	UPROPERTY(VisibleAnywhere, Category = "Spawn")
	TObjectPtr<class UBoxComponent> TriggerBox;

	// one-shot guard so re-entering the box doesn't fire again
	bool bTriggered = false;
};
