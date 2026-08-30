// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyAttack.generated.h"

/**
 * Task that fires the enemy attack (GA_EnemyAttack) from the BehaviorTree
 * the actual activation logic lives in AEnemyCharacter::PerformAttack this Task just calls it
 */
UCLASS()
class WUTHERINGWAVES_API UBTTask_EnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
