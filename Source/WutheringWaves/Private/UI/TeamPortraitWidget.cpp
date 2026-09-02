// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TeamPortraitWidget.h"

#include "AbilitySystemComponent.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "Character/PlayableCharacter.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UTeamPortraitWidget::SetTeam(const TArray<TObjectPtr<APlayableCharacter>>& Team, int32 ActiveIndex)
{
	ActiveSlot = ActiveIndex;

	UImage* Slots[3]   = { Portrait0, Portrait1, Portrait2 };
	UImage* Rings[3]   = { ActiveRing0, ActiveRing1, ActiveRing2 };
	UImage* Numbers[3] = { NumberIcon0, NumberIcon1, NumberIcon2 };

	for (int32 i = 0; i < 3; ++i)
	{
		const bool bHasChar = Team.IsValidIndex(i) && Team[i] != nullptr;

		// portrait brush
		if (UImage* SlotImage = Slots[i])
		{
			UTexture2D* Portrait = (bHasChar && Team[i]->GetCharacterData()) ? Team[i]->GetCharacterData()->Portrait : nullptr;
			if (Portrait)
			{
				SlotImage->SetBrushFromTexture(Portrait);
				SlotImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				SlotImage->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		// active ring: shown only behind the active slot
		if (UImage* Ring = Rings[i])
		{
			Ring->SetVisibility((bHasChar && i == ActiveIndex) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}

		// number badge (1/2/3): shown on the INACTIVE slots (switch hint), hidden on the active one
		if (UImage* Number = Numbers[i])
		{
			Number->SetVisibility((bHasChar && i != ActiveIndex) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	// per-character HP bars: rebind (roster is stable but this is called on every switch) + refresh
	UnbindHealthDelegates();
	BindHealthDelegates(Team);
	RefreshHealthBars();

	// re-apply blink opacity for the new active slot (so a just-switched-to portrait isn't stuck mid-blink)
	SetPortraitsOpacity(bBlinkDim ? 0.3f : 1.f);
}

//========================================================================
// Per-character HP bars
//========================================================================

void UTeamPortraitWidget::BindHealthDelegates(const TArray<TObjectPtr<APlayableCharacter>>& Team)
{
	for (int32 i = 0; i < Team.Num(); ++i)
	{
		UAbilitySystemComponent* ASC = Team[i] ? Team[i]->GetAbilitySystemComponent() : nullptr;
		HealthASCs.Add(ASC);

		if (ASC)
		{
			HpHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(
				UWuWa_AttributeSetBase::GetHpAttribute()).AddUObject(this, &UTeamPortraitWidget::HandleTeamHealthChanged));
			MaxHpHandles.Add(ASC->GetGameplayAttributeValueChangeDelegate(
				UWuWa_AttributeSetBase::GetMaxHpAttribute()).AddUObject(this, &UTeamPortraitWidget::HandleTeamHealthChanged));
		}
		else
		{
			HpHandles.Add(FDelegateHandle());
			MaxHpHandles.Add(FDelegateHandle());
		}
	}
}

void UTeamPortraitWidget::UnbindHealthDelegates()
{
	for (int32 i = 0; i < HealthASCs.Num(); ++i)
	{
		if (UAbilitySystemComponent* ASC = HealthASCs[i].Get())
		{
			if (HpHandles.IsValidIndex(i))
			{
				ASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetHpAttribute()).Remove(HpHandles[i]);
			}
			if (MaxHpHandles.IsValidIndex(i))
			{
				ASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetMaxHpAttribute()).Remove(MaxHpHandles[i]);
			}
		}
	}

	HealthASCs.Reset();
	HpHandles.Reset();
	MaxHpHandles.Reset();
}

void UTeamPortraitWidget::HandleTeamHealthChanged(const FOnAttributeChangeData& /*Data*/)
{
	RefreshHealthBars();
}

void UTeamPortraitWidget::RefreshHealthBars()
{
	UProgressBar* Bars[3] = { HealthBar0, HealthBar1, HealthBar2 };

	for (int32 i = 0; i < 3; ++i)
	{
		if (!Bars[i])
		{
			continue;
		}

		UAbilitySystemComponent* ASC = HealthASCs.IsValidIndex(i) ? HealthASCs[i].Get() : nullptr;
		if (!ASC)
		{
			// no character in this slot -> hide the bar (don't leave an empty 0% bar)
			Bars[i]->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		Bars[i]->SetVisibility(ESlateVisibility::HitTestInvisible);

		const float Hp = ASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetHpAttribute());
		const float MaxHp = ASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxHpAttribute());
		Bars[i]->SetPercent(MaxHp > 0.f ? Hp / MaxHp : 0.f);
	}
}

//========================================================================
// Variation gauge -> portrait blink
//========================================================================

void UTeamPortraitWidget::SetAbilitySystemComponent(UAbilitySystemComponent* InASC)
{
	if (BoundASC == InASC)
	{
		PushVariation();
		return;
	}

	UnbindFromCurrentASC();

	BoundASC = InASC;
	if (!BoundASC)
	{
		return;
	}

	EnergyChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetVariationEnergyAttribute()).AddUObject(this, &UTeamPortraitWidget::HandleVariationChanged);
	MaxEnergyChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetMaxVariationEnergyAttribute()).AddUObject(this, &UTeamPortraitWidget::HandleVariationChanged);

	PushVariation();
}

void UTeamPortraitWidget::NativeDestruct()
{
	SetBlinking(false);
	UnbindFromCurrentASC();
	UnbindHealthDelegates();
	Super::NativeDestruct();
}

void UTeamPortraitWidget::HandleVariationChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushVariation();
}

void UTeamPortraitWidget::PushVariation()
{
	if (!BoundASC)
	{
		return;
	}

	const float Energy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetVariationEnergyAttribute());
	const float MaxEnergy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxVariationEnergyAttribute());
	const float Percent = MaxEnergy > 0.f ? Energy / MaxEnergy : 0.f;

	// full -> blink the inactive portraits (edge-triggered). the donut fill lives on the overlay widget.
	const bool bReady = Percent >= 1.f;
	if (bReady != bVariationReady)
	{
		bVariationReady = bReady;
		SetBlinking(bReady);
	}
}

void UTeamPortraitWidget::UnbindFromCurrentASC()
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetVariationEnergyAttribute()).Remove(EnergyChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetMaxVariationEnergyAttribute()).Remove(MaxEnergyChangedHandle);
	}

	EnergyChangedHandle.Reset();
	MaxEnergyChangedHandle.Reset();
	BoundASC = nullptr;
}

//========================================================================
// Blink (inactive portraits)
//========================================================================

void UTeamPortraitWidget::SetBlinking(bool bOn)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (bOn)
	{
		// toggle the (inactive) portrait opacity every 0.35s -> blink
		World->GetTimerManager().SetTimer(BlinkTimer, this, &UTeamPortraitWidget::ToggleBlink, 0.35f, true);
	}
	else
	{
		World->GetTimerManager().ClearTimer(BlinkTimer);
		bBlinkDim = false;
		SetPortraitsOpacity(1.f);
	}
}

void UTeamPortraitWidget::ToggleBlink()
{
	bBlinkDim = !bBlinkDim;
	SetPortraitsOpacity(bBlinkDim ? 0.3f : 1.f);
}

void UTeamPortraitWidget::SetPortraitsOpacity(float Opacity)
{
	UImage* Slots[3] = { Portrait0, Portrait1, Portrait2 };
	for (int32 i = 0; i < 3; ++i)
	{
		if (!Slots[i])
		{
			continue;
		}
		// the currently-controlled character never blinks - it's marked by the ring instead
		Slots[i]->SetRenderOpacity(i == ActiveSlot ? 1.f : Opacity);
	}
}
