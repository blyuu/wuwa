// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_WeaponVisibility.generated.h"

/**
 * Shows or hides the character's weapon at this montage frame.
 * Put one on the "draw" frame (bShowWeapon = true) and one on the "sheathe" frame (false).
 */
UCLASS(meta = (DisplayName = "Weapon Visibility (show / hide)"))
class WUTHERINGWAVES_API UAN_WeaponVisibility : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	// true = draw (show the weapon), false = sheathe (hide it)
	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bShowWeapon = true;
};
