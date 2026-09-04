// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemySpawnBox.h"

#include "Enemy/EnemyCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AEnemySpawnBox::AEnemySpawnBox()
{
	PrimaryActorTick.bCanEverTick = false;

	// the box is just a placement marker for where the enemy appears - no collision needed
	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	RootComponent = SpawnArea;
	SpawnArea->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// arrow shows which way the spawned enemy will face (rotate this actor to aim it)
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(RootComponent);
#if WITH_EDITORONLY_DATA
	Arrow->ArrowColor = FColor::Green;
	Arrow->ArrowSize = 2.f;
	Arrow->bIsScreenSizeScaled = true;
#endif
}

void AEnemySpawnBox::Spawn()
{
	// one-shot: ignore repeat requests (e.g. player re-enters the trigger)
	if (bHasSpawned)
	{
		return;
	}
	bHasSpawned = true;

	// wait out the delay, then spawn. 0 delay -> spawn right away.
	if (SpawnDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(SpawnTimer, this, &AEnemySpawnBox::SpawnEnemy, SpawnDelay, false);
	}
	else
	{
		SpawnEnemy();
	}
}

void AEnemySpawnBox::SpawnEnemy()
{
	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemySpawnBox] %s has no EnemyClass set"), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// AlwaysSpawn so a slight overlap at the spawn point can't silently cancel the spawn
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// spawn at this box's transform; the enemy auto-possesses (PlacedInWorldOrSpawned) and runs its own
	// BeginPlay (attributes, boss bar, intro) from there
	World->SpawnActor<AEnemyCharacter>(EnemyClass, GetActorTransform(), Params);
}
