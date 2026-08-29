// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "WeaponAnimNotifyState.generated.h"

/**
 * 
 */
UCLASS()
class WUTHERINGWAVES_API UWeaponAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	// 무기 블레이드 두께 (trace radius)
	UPROPERTY(EditAnywhere, Category = "Trace")
	float TraceRadius = 10.f;

	// 히트 시 쏠 게임플레이 이벤트 태그. 비워두면 Event.BaseAttack.Hit(플레이어용)을 쓴다.
	// 적 몽타주에서는 Event.EnemyAttack.Hit으로 지정한다.
	UPROPERTY(EditAnywhere, Category = "Trace", meta = (Categories = "Event"))
	FGameplayTag HitEventTag;

private:
	FVector PrevSocketRoot = FVector::ZeroVector;
	FVector PrevSocketTip  = FVector::ZeroVector;

	// 한 스윙에서 같은 대상에 중복 히트 방지
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
