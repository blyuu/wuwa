// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

/**
 * One floating damage number (screen-space). The HUD spawns it at the projected screen position of the
 * hit and calls SetDamage(); it then floats up + fades on its own and removes itself when done.
 */
UCLASS()
class WUTHERINGWAVES_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// number + color (color chosen from ElementColors by the attacker's element tag)
	void SetDamage(float Amount, const FGameplayTag& ElementTag);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// TextBlock named exactly "DamageText" in the WBP subclass (required)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageText;

	// element tag -> number color. Fill in the WBP defaults (same values as the overlay's ElementColors).
	UPROPERTY(EditAnywhere, Category = "Damage", meta = (Categories = "Character.Element"))
	TMap<FGameplayTag, FLinearColor> ElementColors;

	// seconds the number lives before removing itself
	UPROPERTY(EditAnywhere, Category = "Damage")
	float LifeSeconds = 1.3f;

	// how many screen pixels the number rises over its life
	UPROPERTY(EditAnywhere, Category = "Damage")
	float RiseDistance = 90.f;

private:
	float Elapsed = 0.f;
};
