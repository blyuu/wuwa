// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "WuwaOverlayWidget.generated.h"

struct FOnAttributeChangeData;
class UAbilitySystemComponent;
class UImage;
class UCharacterDataAsset;
class UMaterialInstanceDynamic;
class UTexture2D;
class UTextBlock;

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

	// polls the active character's skill/ultimate cooldowns each frame and drives the cooldown UI
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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

	// variation gauge (donut) next to the health bar
	void HandleVariationChanged(const FOnAttributeChangeData& Data);
	void PushVariation();

	// ultimate gauge (radial fill behind the Liberation icon) - shows how full the ultimate is
	void HandleUltimateChanged(const FOnAttributeChangeData& Data);
	void PushUltimate();

	// tint both gauge materials (variation donut + ultimate) to the active character's element color
	void ApplyElementColor(const FGameplayTag& ElementTag);

	// drive one ability's cooldown UI: radial fill (remaining/duration) + countdown text
	void UpdateCooldown(UImage* Radial, UTextBlock* Text, TObjectPtr<UMaterialInstanceDynamic>& MID, const FGameplayTag& CooldownTag);

	void UnbindFromCurrentASC();

	// Bound by name to Image widgets in the WBP subclass. The WBP MUST contain three
	// Image widgets named exactly like these, or it won't compile (that's the contract).
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BaseAttackIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ResonanceSkillIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LiberationIcon;

	// Variation gauge (도넛) next to the health bar - an Image using a radial-fill material. Optional.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> VariationGauge;

	// Scalar parameter on the gauge material that holds the 0..1 fill.
	UPROPERTY(EditAnywhere, Category = "HUD")
	FName VariationPercentParam = TEXT("Percent");

	// Element icon shown inside the variation donut - picked by the active character's element tag. Optional.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ElementIcon;

	// Element tag -> element icon texture. Fill this in the WBP defaults (one entry per element).
	UPROPERTY(EditAnywhere, Category = "HUD", meta = (Categories = "Character.Element"))
	TMap<FGameplayTag, TObjectPtr<UTexture2D>> ElementIcons;

	// Element tag -> gauge fill color (variation donut + ultimate). Fill this in the WBP defaults.
	UPROPERTY(EditAnywhere, Category = "HUD", meta = (Categories = "Character.Element"))
	TMap<FGameplayTag, FLinearColor> ElementColors;

	// Vector parameter (RGBA) on the gauge materials that holds the fill color.
	UPROPERTY(EditAnywhere, Category = "HUD")
	FName GaugeColorParam = TEXT("EnergyColor");

	// --- Cooldown UI: a radial-fill Image over each icon + a countdown TextBlock. All optional. ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ResonanceSkillCooldown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> LiberationCooldown;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResonanceSkillCooldownText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LiberationCooldownText;

	// Scalar param on the cooldown materials that holds the 0..1 sweep (remaining / duration).
	UPROPERTY(EditAnywhere, Category = "HUD")
	FName CooldownPercentParam = TEXT("Percent");

	// Ultimate gauge - an Image behind the Liberation icon using the same radial-fill material. Optional.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> UltimateGauge;

	// Scalar parameter on the ultimate gauge material that holds the 0..1 fill.
	UPROPERTY(EditAnywhere, Category = "HUD")
	FName UltimatePercentParam = TEXT("Percent");

	// Tint on the Liberation icon while the ultimate gauge isn't full yet (grayed out). White once ready.
	UPROPERTY(EditAnywhere, Category = "HUD")
	FLinearColor UltimateNotReadyTint = FLinearColor(0.3f, 0.3f, 0.3f, 1.f);

	// The ASC we're currently listening to.
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> GaugeMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> UltimateGaugeMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SkillCooldownMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> LiberationCooldownMID;

	FDelegateHandle HpChangedHandle;
	FDelegateHandle MaxHpChangedHandle;
	FDelegateHandle EnergyChangedHandle;
	FDelegateHandle MaxEnergyChangedHandle;
	FDelegateHandle VariationEnergyChangedHandle;
	FDelegateHandle MaxVariationEnergyChangedHandle;
	FDelegateHandle UltimateEnergyChangedHandle;
	FDelegateHandle MaxUltimateEnergyChangedHandle;
};
