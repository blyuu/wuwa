// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyAttack.generated.h"

/**
 * BehaviorTree에서 적의 공격(GA_EnemyAttack)을 발동시키는 Task.
 * 실제 발동 로직은 AEnemyCharacter::PerformAttack에 있고, 이 Task는 그걸 호출만 한다.
 */
UCLASS()
class WUTHERINGWAVES_API UBTTask_EnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
