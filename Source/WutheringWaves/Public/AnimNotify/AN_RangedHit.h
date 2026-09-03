// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_RangedHit.generated.h"

/**
 * Ranged hit judgement for characters whose skill/ultimate isn't weapon-swept.
 * At this montage frame it finds the enemy in front (AcquireTargetEnemy) and fires the same
 * Event.Attack.Hit as a melee swing - so the ability's OnHitEvent applies the damage as usual.
 */
UCLASS(meta = (DisplayName = "Ranged Hit (front enemy)"))
class WUTHERINGWAVES_API UAN_RangedHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
