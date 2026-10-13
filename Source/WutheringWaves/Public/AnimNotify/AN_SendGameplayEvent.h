// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AN_SendGameplayEvent.generated.h"

/**
 * 
 */
UCLASS()
class WUTHERINGWAVES_API UAN_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference) override;
	
private:
	UPROPERTY(EditAnywhere, Category = "Gameplay Ability")
	FGameplayTag EventTag;

	// optional payload for handlers that need to tell apart several notifies sending the same tag
	// (e.g. Event.Skill.Vfx picking which FSkillData::Vfx entry to play). Unused by EventTag
	// handlers that don't read EventMagnitude.
	UPROPERTY(EditAnywhere, Category = "Gameplay Ability")
	int32 EventIndex = 0;
};
