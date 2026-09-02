// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AN_TrackTarget.generated.h"

/**
 * Place this notify-state over the wind-up of each attack inside an enemy attack montage.
 * While it is active, the owning enemy rotates (yaw) toward the current player character,
 * so a multi-hit montage keeps following the player instead of swinging at air.
 * Put one window per attack (ending before the strike) to "aim" each hit but still lock during the swing.
 */
UCLASS(meta = (DisplayName = "Track Target (rotate toward player)"))
class WUTHERINGWAVES_API UAN_TrackTarget : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;

	// higher = snappier turn toward the player (RInterp speed). 0 = instant snap each frame.
	UPROPERTY(EditAnywhere, Category = "Tracking")
	float TurnSpeed = 8.f;

	// stop turning once we're within this many degrees of the player (avoids micro-jitter)
	UPROPERTY(EditAnywhere, Category = "Tracking")
	float AngleDeadzoneDeg = 2.f;
};
