// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TeamPortraitWidget.generated.h"

struct FOnAttributeChangeData;
class APlayableCharacter;
class UAbilitySystemComponent;
class UImage;

/**
 * Right-side team panel: three character portraits (active one drawn larger).
 * Also watches the active character's VariationEnergy so the portraits get a "charged" sparkle
 * when it fills (the donut gauge itself lives on the overlay widget, next to the health bar).
 */
UCLASS()
class WUTHERINGWAVES_API UTeamPortraitWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Set the three portraits and which slot is active (active is scaled up).
	void SetTeam(const TArray<TObjectPtr<APlayableCharacter>>& Team, int32 ActiveIndex);

	// Point at the active character's ASC (watches VariationEnergy to drive the portrait sparkle).
	void SetAbilitySystemComponent(UAbilitySystemComponent* InASC);

protected:
	virtual void NativeDestruct() override;

	// WBP must contain three Image widgets with these exact names.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Portrait0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Portrait1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Portrait2;

	// Scale applied to the active character's portrait.
	UPROPERTY(EditAnywhere, Category = "Team Portrait")
	float ActivePortraitScale = 1.3f;

	// Implement in the WBP: turn the portrait "charged" sparkle on/off when variation becomes full/empty.
	UFUNCTION(BlueprintImplementableEvent, Category = "Team Portrait")
	void OnVariationReadyChanged(bool bReady);

private:
	void HandleVariationChanged(const FOnAttributeChangeData& Data);
	void PushVariation();
	void UnbindFromCurrentASC();

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;

	bool bVariationReady = false;

	FDelegateHandle EnergyChangedHandle;
	FDelegateHandle MaxEnergyChangedHandle;
};
