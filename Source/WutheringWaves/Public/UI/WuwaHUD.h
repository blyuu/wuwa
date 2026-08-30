// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WuwaHUD.generated.h"

class UWuwaOverlayWidget;
class APlayableCharacter;

/**
 * Player HUD. Owns the bottom status overlay and re-points it at whichever
 * character is currently active (initial spawn + every team switch).
 */
UCLASS()
class WUTHERINGWAVES_API AWuwaHUD : public AHUD
{
	GENERATED_BODY()

public:
	// Called by the team system on the initial character and on every switch.
	void OnPlayerCharacterChanged(APlayableCharacter* NewCharacter);

protected:
	virtual void BeginPlay() override;

	// Assign the WBP_Overlay subclass here in the BP_WuwaHUD defaults.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UWuwaOverlayWidget> OverlayClass;

private:
	// Creates the overlay once and adds it to the viewport. Safe to call repeatedly.
	void EnsureOverlay();

	UPROPERTY()
	TObjectPtr<UWuwaOverlayWidget> OverlayWidget;
};
