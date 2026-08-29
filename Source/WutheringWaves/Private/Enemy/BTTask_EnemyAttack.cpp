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

	// 발동 명령을 보냈으니 Task 자체는 즉시 성공 처리한다.
	// (몽타주가 끝날 때까지 기다리는 건 다음 단계에서 Wait 노드나 전용 Task로 처리)
	return EBTNodeResult::Succeeded;
}
