// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "WuwaBossHealthWidget.generated.h"

struct FOnAttributeChangeData;
class UAbilitySystemComponent;
class UEnemyDataAsset;
class UProgressBar;
class UTextBlock;

/**
 * Boss health bar shown at the top of the screen while a boss is alive.
 *
 * This C++ base drives the widgets directly via BindWidget: it subscribes to the
 * boss ASC's Hp/Groggy attribute-change delegates and writes straight into the
 * bound ProgressBars / TextBlocks. The WBP subclass only has to lay them out and
 * name them exactly like the properties below.
 */
UCLASS()
class WUTHERINGWAVES_API UWuwaBossHealthWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Point this bar at the boss's ASC: rebinds the delegates and seeds current values.
	void SetAbilitySystemComponent(UAbilitySystemComponent* InASC);

	// Static data (name + level) pulled from the boss's data asset.
	void SetBossInfo(const UEnemyDataAsset* Data);

protected:
	virtual void NativeDestruct() override;

	//========================================================================
	// The WBP subclass MUST contain widgets named EXACTLY like these, with these
	// types, or the WBP won't compile. That's the BindWidget contract.
	//========================================================================
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossLevelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> GroggyBar;

	// GroggyBar fill tint. Normal while the gauge drains; switches to the recover color (yellow) while
	// the enemy is staggered, so the same bar shows the recovery fill.
	UPROPERTY(EditAnywhere, Category = "Boss HUD")
	FLinearColor GroggyBarColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Boss HUD")
	FLinearColor GroggyRecoverColor = FLinearColor(1.f, 0.85f, 0.1f, 1.f);

private:
	// ASC delegate callbacks. They ignore the delta and just re-read the pair so the
	// bar always gets both current + max together.
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);
	void HandleGroggyChanged(const FOnAttributeChangeData& Data);
	void HandleMaxGroggyChanged(const FOnAttributeChangeData& Data);

	// Fires when Enemy.State.Groggy is added/removed -> swap the groggy bar for the yellow recovery gauge.
	void HandleGroggyStateChanged(const FGameplayTag Tag, int32 NewCount);
	void SetRecovering(bool bRecovering);

	void PushHealth();
	void PushGroggy();

	void UnbindFromCurrentASC();

	// The ASC we're currently listening to (the boss).
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;

	FDelegateHandle HpChangedHandle;
	FDelegateHandle MaxHpChangedHandle;
	FDelegateHandle GroggyChangedHandle;
	FDelegateHandle MaxGroggyChangedHandle;
	FDelegateHandle GroggyStateHandle;
};
