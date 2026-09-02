// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TeamPortraitWidget.generated.h"

struct FOnAttributeChangeData;
class APlayableCharacter;
class UAbilitySystemComponent;
class UImage;
class UProgressBar;

/**
 * Right-side team panel: three portraits, each with a white HP bar. The active character is marked by a
 * ring image shown BEHIND its portrait (not by scaling). Also watches the active character's
 * VariationEnergy so the inactive portraits blink when it fills.
 */
UCLASS()
class WUTHERINGWAVES_API UTeamPortraitWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Set the three portraits + HP bars + which slot is active (active shows the ring behind it).
	void SetTeam(const TArray<TObjectPtr<APlayableCharacter>>& Team, int32 ActiveIndex);

	// Point at the active character's ASC (watches VariationEnergy to drive the portrait blink).
	void SetAbilitySystemComponent(UAbilitySystemComponent* InASC);

protected:
	virtual void NativeDestruct() override;

	// Portraits (required) - WBP must contain Image widgets with these exact names.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Portrait0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Portrait1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Portrait2;

	// Per-character HP bars (style them white in the WBP). Optional.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar2;

	// Ring image shown BEHIND the active character's portrait. Optional.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ActiveRing0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ActiveRing1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ActiveRing2;

	// Slot number badge (1 / 2 / 3). Shown on the INACTIVE slots as a switch hint, hidden on the active slot.
	// Put the "1" image in NumberIcon0, "2" in NumberIcon1, "3" in NumberIcon2 in the WBP. Optional.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> NumberIcon0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> NumberIcon1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> NumberIcon2;

private:
	// variation gauge -> portrait blink (donut fill itself lives on the overlay widget)
	void HandleVariationChanged(const FOnAttributeChangeData& Data);
	void PushVariation();
	void UnbindFromCurrentASC();

	// per-character HP bars
	void RefreshHealthBars();
	void HandleTeamHealthChanged(const FOnAttributeChangeData& Data);
	void BindHealthDelegates(const TArray<TObjectPtr<APlayableCharacter>>& Team);
	void UnbindHealthDelegates();

	// blink the INACTIVE portraits while variation is full (timer-driven, no WBP animation)
	void SetBlinking(bool bOn);
	void ToggleBlink();
	void SetPortraitsOpacity(float Opacity);

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;

	bool bVariationReady = false;
	bool bBlinkDim = false;
	int32 ActiveSlot = 0;   // the currently-controlled slot - excluded from the blink
	FTimerHandle BlinkTimer;

	FDelegateHandle EnergyChangedHandle;
	FDelegateHandle MaxEnergyChangedHandle;

	// health delegate bookkeeping (parallel arrays, one entry per team slot)
	TArray<TWeakObjectPtr<UAbilitySystemComponent>> HealthASCs;
	TArray<FDelegateHandle> HpHandles;
	TArray<FDelegateHandle> MaxHpHandles;
};
