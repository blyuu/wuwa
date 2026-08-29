// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_UpdateTarget.generated.h"

/**
 * 주기적으로 플레이어 폰을 찾아 Blackboard의 TargetActor에 기록하고,
 * 적과의 거리를 재서 InAttackRange(Bool)를 갱신하는 Service.
 */
UCLASS()
class WUTHERINGWAVES_API UBTService_UpdateTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateTarget();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 감지한 플레이어를 담을 Blackboard 키 (Object)
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector TargetActorKey;

	// 공격 사거리 안인지 여부를 담을 Blackboard 키 (Bool)
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector InAttackRangeKey;

	// 이 거리 이내면 InAttackRange = true. 몬스터/공격에 맞춰 튜닝.
	UPROPERTY(EditAnywhere, Category = "AI")
	float AttackRange = 200.f;
};
