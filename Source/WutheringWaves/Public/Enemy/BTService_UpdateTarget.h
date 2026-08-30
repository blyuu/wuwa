// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_UpdateTarget.generated.h"

/**
 * Service that periodically finds the player pawn and writes it to the Blackboard's TargetActor
 * and measures the distance to the enemy to update InAttackRange (Bool)
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

	// Blackboard key to hold the detected player (Object)
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector TargetActorKey;

	// Blackboard key to hold whether we're in attack range (Bool)
	UPROPERTY(EditAnywhere, Category = "AI")
	FBlackboardKeySelector InAttackRangeKey;

	// within this distance InAttackRange = true tune per monster/attack
	UPROPERTY(EditAnywhere, Category = "AI")
	float AttackRange = 200.f;
};
