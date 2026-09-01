// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WuwaHUD.generated.h"

class UWuwaOverlayWidget;
class UWuwaBossHealthWidget;
class UTeamPortraitWidget;
class APlayableCharacter;
class AEnemyCharacter;

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

	// Boss bar: shown when a boss spawns, hidden when it dies. Called by AEnemyCharacter.
	void ShowBossBar(AEnemyCharacter* Boss);
	void HideBossBar();

protected:
	virtual void BeginPlay() override;

	// Assign the WBP_Overlay subclass here in the BP_WuwaHUD defaults.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UWuwaOverlayWidget> OverlayClass;

	// Assign the WBP_BossHealth subclass here in the BP_WuwaHUD defaults.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UWuwaBossHealthWidget> BossHealthClass;

	// Assign the WBP_TeamPortrait subclass here in the BP_WuwaHUD defaults.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UTeamPortraitWidget> TeamPortraitClass;

private:
	// Creates the overlay once and adds it to the viewport. Safe to call repeatedly.
	void EnsureOverlay();

	// Creates the team portrait panel once. Safe to call repeatedly.
	void EnsureTeamPortrait();

	UPROPERTY()
	TObjectPtr<UWuwaOverlayWidget> OverlayWidget;

	UPROPERTY()
	TObjectPtr<UWuwaBossHealthWidget> BossWidget;

	UPROPERTY()
	TObjectPtr<UTeamPortraitWidget> TeamPortraitWidget;
};
