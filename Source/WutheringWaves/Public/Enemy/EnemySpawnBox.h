// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawnBox.generated.h"

// Marks WHERE and WHAT to spawn. It is not a trigger itself - a AEnemySpawnTrigger calls Spawn() on it.
// Place it where the enemy should appear; the enemy spawns at this box's location, facing the arrow.
UCLASS()
class WUTHERINGWAVES_API AEnemySpawnBox : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawnBox();

	// enemy (BP) to spawn here. Its EnemyDataAsset/BehaviorTree must be set on the BP's CLASS defaults
	// (not just on a placed instance), since we spawn from the class.
	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<class AEnemyCharacter> EnemyClass;

	// seconds between the trigger firing and the enemy actually appearing (the "drop-in" beat)
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.0"))
	float SpawnDelay = 1.f;

	// called by the trigger: spawn the enemy here after SpawnDelay. One-shot - later calls are ignored.
	void Spawn();

protected:
	// does the actual SpawnActor at this box's transform
	void SpawnEnemy();

	// visual box marking the spawn spot (root). No collision - it's just a placement marker.
	UPROPERTY(VisibleAnywhere, Category = "Spawn")
	TObjectPtr<class UBoxComponent> SpawnArea;

	// which way the spawned enemy faces (rotate this actor to aim it)
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UArrowComponent> Arrow;

	// one-shot guard: true once we've spawned, so the boss can't be spawned twice
	bool bHasSpawned = false;

	// handle for the delayed spawn timer
	FTimerHandle SpawnTimer;
};
