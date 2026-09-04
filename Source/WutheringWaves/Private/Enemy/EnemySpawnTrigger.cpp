// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemySpawnTrigger.h"

#include "Enemy/EnemySpawnBox.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"

AEnemySpawnTrigger::AEnemySpawnTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	// the box is the root and the trigger: move/resize it in the editor to set the trigger area.
	// Set it to only overlap pawns so nothing else trips it.
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;
	TriggerBox->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AEnemySpawnTrigger::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AEnemySpawnTrigger::OnTriggerOverlap);
}

void AEnemySpawnTrigger::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// one-shot: ignore everything after the first valid trigger
	if (bTriggered)
	{
		return;
	}

	// only the player's active character should start the encounter (team members aren't player-controlled)
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	if (!SpawnBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemySpawnTrigger] %s has no SpawnBox linked"), *GetName());
		return;
	}

	bTriggered = true;
	SpawnBox->Spawn();
}
