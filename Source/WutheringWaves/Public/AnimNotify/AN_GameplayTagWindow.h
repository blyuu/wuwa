// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AN_GameplayTagWindow.generated.h"

/**
 * Adds a loose gameplay tag to the owner's ASC for the length of this notify window,
 * then removes it when the window ends. One reusable notify for two jobs:
 *   - Enemy attack montage danger window  -> TagToApply = Enemy.State.Attacking (lets the player detect a perfect dodge)
 *   - Player dodge i-frame window          -> TagToApply = State.Invulnerable   (damage GE checks this and is skipped)
 */
UCLASS()
class WUTHERINGWAVES_API UAN_GameplayTagWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

protected:
	// The tag held on the owner's ASC while this window is active. Set per-montage in the editor.
	UPROPERTY(EditAnywhere, Category = "GameplayTag Window")
	FGameplayTag TagToApply;
};
