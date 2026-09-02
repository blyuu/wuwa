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

	// weapon blade thickness (trace radius)
	UPROPERTY(EditAnywhere, Category = "Trace")
	float TraceRadius = 10.f;

	// gameplay event tag to fire on hit. if left empty it uses Event.Attack.Hit (player base attack / skill / ultimate)
	// on enemy montages set this to Event.EnemyAttack.Hit
	UPROPERTY(EditAnywhere, Category = "Trace", meta = (Categories = "Event"))
	FGameplayTag HitEventTag;

private:
	FVector PrevSocketRoot = FVector::ZeroVector;
	FVector PrevSocketTip  = FVector::ZeroVector;

	// prevents hitting the same target twice in one swing
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
