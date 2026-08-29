// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/WuwaEnemyController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "Enemy/EnemyCharacter.h"

void AWuwaEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// BT 참조는 적(BP_Enemy)이 들고 있다 -> possess한 폰에서 읽어와 실행한다.
	// (적 종류마다 다른 BT를 줄 수 있고, 컨트롤러는 종류를 몰라도 된다)
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(InPawn);
	if (Enemy && Enemy->BehaviorTree)
	{
		// RunBehaviorTree는 BT에 지정된 Blackboard 애셋으로 Blackboard 컴포넌트도 자동 초기화한다.
		RunBehaviorTree(Enemy->BehaviorTree);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyController] No BehaviorTree set on %s"),
			InPawn ? *InPawn->GetName() : TEXT("null"));
	}
}
