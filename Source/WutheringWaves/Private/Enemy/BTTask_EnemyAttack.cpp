// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BTTask_EnemyAttack.h"

#include "AIController.h"
#include "Enemy/EnemyCharacter.h"

UBTTask_EnemyAttack::UBTTask_EnemyAttack()
{
	NodeName = TEXT("Enemy Attack");
}

EBTNodeResult::Type UBTTask_EnemyAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon)
	{
		return EBTNodeResult::Failed;
	}

	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(AICon->GetPawn());
	if (!Enemy)
	{
		return EBTNodeResult::Failed;
	}

	Enemy->PerformAttack();

	// we sent the activate command so the Task itself succeeds right away
	// (waiting until the montage ends is handled later by a Wait node or a dedicated Task)
	return EBTNodeResult::Succeeded;
}
