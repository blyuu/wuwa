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

	// hit forgiveness: sphere radius swept along the blade. Bigger = more generous ("this close counts as a hit").
	UPROPERTY(EditAnywhere, Category = "Trace")
	float TraceRadius = 30.f;

	// gameplay event tag to fire on hit. if left empty it uses Event.Attack.Hit (player base attack / skill / ultimate)
	// on enemy montages set this to Event.EnemyAttack.Hit
	UPROPERTY(EditAnywhere, Category = "Trace", meta = (Categories = "Event"))
	FGameplayTag HitEventTag;

	// "game-feel" correction: if the precise blade sweep connected with nobody the whole swing, still count a
	// hit on the target the attacker is clearly facing (player -> AcquireTargetEnemy, enemy -> the player).
	UPROPERTY(EditAnywhere, Category = "Trace|Fallback")
	bool bUseFallbackHit = true;

	// the fallback only fires on a target within this distance of the attacker
	UPROPERTY(EditAnywhere, Category = "Trace|Fallback")
	float FallbackRange = 200.f;

	// ...and within this half-angle of the attacker's forward, so we never "correct" onto something behind
	UPROPERTY(EditAnywhere, Category = "Trace|Fallback")
	float FallbackConeHalfAngleDeg = 75.f;

private:
	// registers a hit on the faced target when the sweep missed (see bUseFallbackHit)
	void TryFallbackHit(USkeletalMeshComponent* MeshComp);

	FVector PrevSocketRoot = FVector::ZeroVector;
	FVector PrevSocketTip  = FVector::ZeroVector;

	// prevents hitting the same target twice in one swing
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
