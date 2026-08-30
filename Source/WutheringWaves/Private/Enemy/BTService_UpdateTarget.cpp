// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/BTService_UpdateTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
	NodeName = TEXT("Update Target");

	// update every 0.3s (±0.05) no need to run every frame
	Interval = 0.3f;
	RandomDeviation = 0.05f;
	bNotifyTick = true;

	// filter so the editor dropdown only shows keys of the right type
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, TargetActorKey), AActor::StaticClass());
	InAttackRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, InAttackRangeKey));
}

void UBTService_UpdateTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// resolve the key selectors against this BT's Blackboard asset
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		InAttackRangeKey.ResolveSelectedKey(*BBAsset);
	}
}

void UBTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!BB || !AICon)
	{
		return;
	}

	APawn* EnemyPawn = AICon->GetPawn();
	if (!EnemyPawn)
	{
		return;
	}

	// the currently controlled player pawn even if team swap (1/2/3) changes it we grab the latest each time
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(EnemyPawn, 0);
	if (!PlayerPawn)
	{
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
		BB->SetValueAsBool(InAttackRangeKey.SelectedKeyName, false);
		return;
	}

	BB->SetValueAsObject(TargetActorKey.SelectedKeyName, PlayerPawn);

	const float Distance = FVector::Dist(EnemyPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
	BB->SetValueAsBool(InAttackRangeKey.SelectedKeyName, Distance <= AttackRange);
}
