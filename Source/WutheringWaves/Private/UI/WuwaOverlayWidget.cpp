// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WuwaOverlayWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "DataAsset/CharacterDataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"

namespace
{
	// Look up one ability's icon in the data asset and push it into the given Image widget.
	void ApplyIcon(UImage* Image, const UCharacterDataAsset* Data, const FGameplayTag& Tag)
	{
		if (!Image)
		{
			return;
		}

		if (const FSkillData* Skill = Data->Skills.Find(Tag))
		{
			if (Skill->Icon)
			{
				Image->SetBrushFromTexture(Skill->Icon);
				Image->SetVisibility(ESlateVisibility::HitTestInvisible);
				return;
			}
		}

		// No icon configured for this ability -> hide the slot instead of showing an empty box.
		Image->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWuwaOverlayWidget::SetAbilitySystemComponent(UAbilitySystemComponent* InASC)
{
	// Same target: just refresh the values (e.g. attributes were re-initialized) and bail.
	if (BoundASC == InASC)
	{
		PushHealth();
		PushResonanceEnergy();
		return;
	}

	UnbindFromCurrentASC();

	BoundASC = InASC;
	if (!BoundASC)
	{
		return;
	}

	HpChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetHpAttribute()).AddUObject(this, &UWuwaOverlayWidget::HandleHealthChanged);
	MaxHpChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetMaxHpAttribute()).AddUObject(this, &UWuwaOverlayWidget::HandleMaxHealthChanged);
	EnergyChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetResonanceEnergyAttribute()).AddUObject(this, &UWuwaOverlayWidget::HandleResonanceEnergyChanged);
	MaxEnergyChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetMaxResonanceEnergyAttribute()).AddUObject(this, &UWuwaOverlayWidget::HandleMaxResonanceEnergyChanged);

	VariationEnergyChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetVariationEnergyAttribute()).AddUObject(this, &UWuwaOverlayWidget::HandleVariationChanged);
	MaxVariationEnergyChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetMaxVariationEnergyAttribute()).AddUObject(this, &UWuwaOverlayWidget::HandleVariationChanged);

	UltimateEnergyChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetUltimateEnergyAttribute()).AddUObject(this, &UWuwaOverlayWidget::HandleUltimateChanged);
	MaxUltimateEnergyChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(
		UWuWa_AttributeSetBase::GetMaxUltimateEnergyAttribute()).AddUObject(this, &UWuwaOverlayWidget::HandleUltimateChanged);

	// Delegates only fire on *future* changes, so seed the UI with the current values now.
	PushHealth();
	PushResonanceEnergy();
	PushVariation();
	PushUltimate();
}

void UWuwaOverlayWidget::SetLevel(int32 NewLevel)
{
	OnLevelChanged(NewLevel);
}

void UWuwaOverlayWidget::SetSkillIcons(const UCharacterDataAsset* Data)
{
	if (!Data)
	{
		return;
	}

	ApplyIcon(BaseAttackIcon, Data, AbilityTags::Ability_Type_BaseAttack);
	ApplyIcon(ResonanceSkillIcon, Data, AbilityTags::Ability_Type_ResonanceSkill);
	ApplyIcon(LiberationIcon, Data, AbilityTags::Ability_Type_Liberation);
}

void UWuwaOverlayWidget::NativeDestruct()
{
	UnbindFromCurrentASC();
	Super::NativeDestruct();
}

void UWuwaOverlayWidget::HandleHealthChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushHealth();
}

void UWuwaOverlayWidget::HandleMaxHealthChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushHealth();
}

void UWuwaOverlayWidget::HandleResonanceEnergyChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushResonanceEnergy();
}

void UWuwaOverlayWidget::HandleMaxResonanceEnergyChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushResonanceEnergy();
}

void UWuwaOverlayWidget::PushHealth()
{
	if (!BoundASC)
	{
		return;
	}

	const float Hp = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetHpAttribute());
	const float MaxHp = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxHpAttribute());
	OnHealthChanged(Hp, MaxHp);
}

void UWuwaOverlayWidget::PushResonanceEnergy()
{
	if (!BoundASC)
	{
		return;
	}

	const float Energy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetResonanceEnergyAttribute());
	const float MaxEnergy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxResonanceEnergyAttribute());
	OnResonanceEnergyChanged(Energy, MaxEnergy);
}

void UWuwaOverlayWidget::HandleVariationChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushVariation();
}

void UWuwaOverlayWidget::PushVariation()
{
	if (!BoundASC || !VariationGauge)
	{
		return;
	}

	const float Energy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetVariationEnergyAttribute());
	const float MaxEnergy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxVariationEnergyAttribute());
	const float Percent = MaxEnergy > 0.f ? Energy / MaxEnergy : 0.f;

	if (!GaugeMID)
	{
		GaugeMID = VariationGauge->GetDynamicMaterial(); // makes a MID from the image's material
	}
	if (GaugeMID)
	{
		GaugeMID->SetScalarParameterValue(VariationPercentParam, Percent);
	}
}

void UWuwaOverlayWidget::HandleUltimateChanged(const FOnAttributeChangeData& /*Data*/)
{
	PushUltimate();
}

void UWuwaOverlayWidget::PushUltimate()
{
	if (!BoundASC || !UltimateGauge)
	{
		return;
	}

	const float Energy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetUltimateEnergyAttribute());
	const float MaxEnergy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxUltimateEnergyAttribute());
	const float Percent = MaxEnergy > 0.f ? Energy / MaxEnergy : 0.f;

	if (!UltimateGaugeMID)
	{
		UltimateGaugeMID = UltimateGauge->GetDynamicMaterial(); // MID from the ultimate gauge image's material
	}
	if (UltimateGaugeMID)
	{
		UltimateGaugeMID->SetScalarParameterValue(UltimatePercentParam, Percent);
	}
}

void UWuwaOverlayWidget::UnbindFromCurrentASC()
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetHpAttribute()).Remove(HpChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetMaxHpAttribute()).Remove(MaxHpChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetResonanceEnergyAttribute()).Remove(EnergyChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetMaxResonanceEnergyAttribute()).Remove(MaxEnergyChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetVariationEnergyAttribute()).Remove(VariationEnergyChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetMaxVariationEnergyAttribute()).Remove(MaxVariationEnergyChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetUltimateEnergyAttribute()).Remove(UltimateEnergyChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetMaxUltimateEnergyAttribute()).Remove(MaxUltimateEnergyChangedHandle);
	}

	HpChangedHandle.Reset();
	MaxHpChangedHandle.Reset();
	EnergyChangedHandle.Reset();
	MaxEnergyChangedHandle.Reset();
	VariationEnergyChangedHandle.Reset();
	MaxVariationEnergyChangedHandle.Reset();
	UltimateEnergyChangedHandle.Reset();
	MaxUltimateEnergyChangedHandle.Reset();
	BoundASC = nullptr;
}
