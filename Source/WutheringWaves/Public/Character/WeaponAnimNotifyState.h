// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
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

private:
	FVector PrevSocketRoot = FVector::ZeroVector;
	FVector PrevSocketTip  = FVector::ZeroVector;

	// 한 스윙에서 같은 대상에 중복 히트 방지
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
