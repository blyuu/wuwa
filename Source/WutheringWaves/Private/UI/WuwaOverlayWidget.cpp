// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WuwaOverlayWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Image.h"
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

	// Delegates only fire on *future* changes, so seed the UI with the current values now.
	PushHealth();
	PushResonanceEnergy();
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

void UWuwaOverlayWidget::UnbindFromCurrentASC()
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetHpAttribute()).Remove(HpChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetMaxHpAttribute()).Remove(MaxHpChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetResonanceEnergyAttribute()).Remove(EnergyChangedHandle);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UWuWa_AttributeSetBase::GetMaxResonanceEnergyAttribute()).Remove(MaxEnergyChangedHandle);
	}

	HpChangedHandle.Reset();
	MaxHpChangedHandle.Reset();
	EnergyChangedHandle.Reset();
	MaxEnergyChangedHandle.Reset();
	BoundASC = nullptr;
}
