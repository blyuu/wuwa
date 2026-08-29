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

	// 매 0.3초(±0.05)마다 갱신. 매 프레임 돌 필요는 없다.
	Interval = 0.3f;
	RandomDeviation = 0.05f;
	bNotifyTick = true;

	// 에디터 드롭다운이 알맞은 타입의 키만 보여주도록 필터를 건다.
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, TargetActorKey), AActor::StaticClass());
	InAttackRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateTarget, InAttackRangeKey));
}

void UBTService_UpdateTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// 키 선택자를 이 BT의 Blackboard 애셋에 연결(resolve)한다.
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

	// 현재 조종 중인 플레이어 폰. 팀 교체(1/2/3)로 폰이 바뀌어도 매번 최신 폰을 가져온다.
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
