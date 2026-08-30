// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/WuwaEnemyController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "Enemy/EnemyCharacter.h"

void AWuwaEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// the BT reference lives on the enemy (BP_Enemy) -> read it from the possessed pawn and run it
	// (each enemy type can have a different BT and the controller doesn't need to know the type)
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(InPawn);
	if (Enemy && Enemy->BehaviorTree)
	{
		// RunBehaviorTree also auto inits the Blackboard component from the Blackboard asset set on the BT
		RunBehaviorTree(Enemy->BehaviorTree);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyController] No BehaviorTree set on %s"),
			InPawn ? *InPawn->GetName() : TEXT("null"));
	}
}
