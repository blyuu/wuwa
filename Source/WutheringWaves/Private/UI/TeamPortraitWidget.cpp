// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/TeamPortraitWidget.h"

#include "AbilitySystemComponent.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "Character/PlayableCharacter.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Components/Image.h"

void UTeamPortraitWidget::SetTeam(const TArray<TObjectPtr<APlayableCharacter>>& Team, int32 ActiveIndex)
{
	UImage* Slots[3] = { Portrait0, Portrait1, Portrait2 };

	for (int32 i = 0; i < 3; ++i)
	{
		UImage* SlotImage = Slots[i];
		if (!SlotImage)
		{
			continue;
		}

		UTexture2D* Portrait = nullptr;
		if (Team.IsValidIndex(i) && Team[i] && Team[i]->GetCharacterData())
		{
			Portrait = Team[i]->GetCharacterData()->Portrait;
		}

		if (Portrait)
		{
			SlotImage->SetBrushFromTexture(Portrait);
			SlotImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			// active portrait is drawn larger
			SlotImage->SetRenderScale(FVector2D(i == ActiveIndex ? ActivePortraitScale : 1.f));
		}
		else
		{
			SlotImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

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
	UnbindFromCurrentASC();
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

	// full -> portraits sparkle (edge-triggered so the WBP anim only fires on change).
	// the donut fill itself lives on the overlay widget next to the health bar.
	const bool bReady = Percent >= 1.f;
	if (bReady != bVariationReady)
	{
		bVariationReady = bReady;
		OnVariationReadyChanged(bReady);
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
