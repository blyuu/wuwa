// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WuwaOverlayWidget.generated.h"

struct FOnAttributeChangeData;
class UAbilitySystemComponent;
class UImage;
class UCharacterDataAsset;

/**
 * Bottom-of-screen player status bar (HP / Level / Resonance energy).
 *
 * This C++ base only holds the logic: it subscribes to the active character's
 * ASC attribute-change delegates and forwards raw values to the WBP subclass,
 * which owns all the visuals. On a team switch the HUD re-points this widget at
 * the new character via SetAbilitySystemComponent().
 */
UCLASS()
class WUTHERINGWAVES_API UWuwaOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Point this overlay at a character's ASC: rebinds the delegates and seeds the current values.
	// Pass nullptr to just detach (e.g. character destroyed).
	void SetAbilitySystemComponent(UAbilitySystemComponent* InASC);

	// Level is a static data-asset value (no leveling system), so it's pushed straight in.
	void SetLevel(int32 NewLevel);

	// Pulls the character's per-ability icons from its data asset into the bound Image widgets.
	void SetSkillIcons(const UCharacterDataAsset* Data);

protected:
	virtual void NativeDestruct() override;

	//========================================================================
	// Implement these in the WBP subclass to actually update the widgets.
	//========================================================================
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHealthChanged(float Health, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnResonanceEnergyChanged(float Energy, float MaxEnergy);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnLevelChanged(int32 Level);

private:
	// ASC delegate callbacks. They ignore the delta and just re-read the pair so the
	// bar always gets both current + max together.
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);
	void HandleResonanceEnergyChanged(const FOnAttributeChangeData& Data);
	void HandleMaxResonanceEnergyChanged(const FOnAttributeChangeData& Data);

	void PushHealth();
	void PushResonanceEnergy();

	void UnbindFromCurrentASC();

	// Bound by name to Image widgets in the WBP subclass. The WBP MUST contain three
	// Image widgets named exactly like these, or it won't compile (that's the contract).
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BaseAttackIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ResonanceSkillIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LiberationIcon;

	// The ASC we're currently listening to.
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;

	FDelegateHandle HpChangedHandle;
	FDelegateHandle MaxHpChangedHandle;
	FDelegateHandle EnergyChangedHandle;
	FDelegateHandle MaxEnergyChangedHandle;
};
