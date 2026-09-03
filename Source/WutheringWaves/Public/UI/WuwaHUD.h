// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WuwaHUD.generated.h"

class UWuwaOverlayWidget;
class UWuwaBossHealthWidget;
class UTeamPortraitWidget;
class UDamageNumberWidget;
class APlayableCharacter;
class AEnemyCharacter;
struct FCombatFeedbackEvent;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Assign the WBP_Overlay subclass here in the BP_WuwaHUD defaults.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UWuwaOverlayWidget> OverlayClass;

	// Assign the WBP_BossHealth subclass here in the BP_WuwaHUD defaults.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UWuwaBossHealthWidget> BossHealthClass;

	// Assign the WBP_TeamPortrait subclass here in the BP_WuwaHUD defaults.
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UTeamPortraitWidget> TeamPortraitClass;

	// Assign the WBP_DamageNumber subclass here - one is spawned per hit (floating damage number).
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UDamageNumberWidget> DamageNumberClass;

private:
	// Creates the overlay once and adds it to the viewport. Safe to call repeatedly.
	void EnsureOverlay();

	// Creates the team portrait panel once. Safe to call repeatedly.
	void EnsureTeamPortrait();

	// Combat feedback: spawn a floating damage number at the projected screen position of the hit.
	void HandleDamageEvent(const FCombatFeedbackEvent& Event);

	FDelegateHandle DamageDelegateHandle;

	UPROPERTY()
	TObjectPtr<UWuwaOverlayWidget> OverlayWidget;

	UPROPERTY()
	TObjectPtr<UWuwaBossHealthWidget> BossWidget;

	UPROPERTY()
	TObjectPtr<UTeamPortraitWidget> TeamPortraitWidget;
};
