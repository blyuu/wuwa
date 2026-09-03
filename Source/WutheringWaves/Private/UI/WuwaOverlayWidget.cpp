// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WuwaOverlayWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
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
				// Icon may be a Texture2D or a Paper2D Sprite - SetBrushResourceObject handles both.
				Image->SetBrushResourceObject(Skill->Icon);
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

	// element icon inside the variation donut - chosen by the character's element tag (from its data asset)
	if (ElementIcon)
	{
		const TObjectPtr<UTexture2D>* Found = ElementIcons.Find(Data->ElementTag);
		if (Found && *Found)
		{
			ElementIcon->SetBrushFromTexture(*Found);
			ElementIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			// no icon mapped for this element -> hide the slot
			ElementIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// per-character 공명 회로 fill: swap the resonance bar's FillImage brush to this character's texture
	if (ResonanceEnergyBar && Data->ResonanceCircuitIcon)
	{
		FProgressBarStyle Style = ResonanceEnergyBar->GetWidgetStyle();
		Style.FillImage.SetResourceObject(Data->ResonanceCircuitIcon);
		ResonanceEnergyBar->SetWidgetStyle(Style);
	}

	// tint the gauges (variation donut + ultimate) to match the character's element
	ApplyElementColor(Data->ElementTag);
}

void UWuwaOverlayWidget::ApplyElementColor(const FGameplayTag& ElementTag)
{
	// no mapping -> fall back to white (the material's default)
	const FLinearColor* Found = ElementColors.Find(ElementTag);
	const FLinearColor Color = Found ? *Found : FLinearColor::White;

	if (VariationGauge)
	{
		if (!GaugeMID)
		{
			GaugeMID = VariationGauge->GetDynamicMaterial();
		}
		if (GaugeMID)
		{
			GaugeMID->SetVectorParameterValue(GaugeColorParam, Color);
		}
	}

	if (UltimateGauge)
	{
		if (!UltimateGaugeMID)
		{
			UltimateGaugeMID = UltimateGauge->GetDynamicMaterial();
		}
		if (UltimateGaugeMID)
		{
			UltimateGaugeMID->SetVectorParameterValue(GaugeColorParam, Color);
		}
	}
}

void UWuwaOverlayWidget::NativeDestruct()
{
	UnbindFromCurrentASC();
	Super::NativeDestruct();
}

void UWuwaOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// cooldowns are time-based, so poll them each frame (attribute delegates don't fire per-tick)
	UpdateCooldown(ResonanceSkillCooldown, ResonanceSkillCooldownText, SkillCooldownMID, CooldownTags::Cooldown_ResonanceSkill);
	UpdateCooldown(LiberationCooldown, LiberationCooldownText, LiberationCooldownMID, CooldownTags::Cooldown_Liberation);
}

void UWuwaOverlayWidget::UpdateCooldown(UImage* Radial, UTextBlock* Text, TObjectPtr<UMaterialInstanceDynamic>& MID, const FGameplayTag& CooldownTag)
{
	if (!Radial && !Text)
	{
		return;
	}

	// find the longest-remaining active effect that grants this cooldown tag
	float Remaining = 0.f;
	float Duration = 0.f;
	if (BoundASC && CooldownTag.IsValid())
	{
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CooldownTag));
		for (const TPair<float, float>& Pair : BoundASC->GetActiveEffectsTimeRemainingAndDuration(Query))
		{
			Remaining = FMath::Max(Remaining, Pair.Key);
			Duration = FMath::Max(Duration, Pair.Value);
		}
	}

	const bool bOnCooldown = Remaining > 0.f && Duration > 0.f;

	// radial sweep: Percent = remaining / duration (full at cast, empties as it ticks down)
	if (Radial)
	{
		if (bOnCooldown)
		{
			if (!MID)
			{
				MID = Radial->GetDynamicMaterial();
			}
			if (MID)
			{
				MID->SetScalarParameterValue(CooldownPercentParam, Remaining / Duration);
			}
			Radial->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Radial->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// countdown text (e.g. 12.0 -> 11.9)
	if (Text)
	{
		if (bOnCooldown)
		{
			Text->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Remaining)));
			Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
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

	// drive the bar fill directly (the FillImage was set per character in SetSkillIcons)
	if (ResonanceEnergyBar)
	{
		ResonanceEnergyBar->SetPercent(MaxEnergy > 0.f ? Energy / MaxEnergy : 0.f);
	}

	// keep the BP hook too, for any extra visuals the WBP layers on
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
	if (!BoundASC)
	{
		return;
	}

	const float Energy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetUltimateEnergyAttribute());
	const float MaxEnergy = BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxUltimateEnergyAttribute());
	const float Percent = MaxEnergy > 0.f ? Energy / MaxEnergy : 0.f;
	const bool bReady = MaxEnergy > 0.f && Energy >= MaxEnergy - 0.01f;

	// radial fill behind the icon
	if (UltimateGauge)
	{
		if (!UltimateGaugeMID)
		{
			UltimateGaugeMID = UltimateGauge->GetDynamicMaterial(); // MID from the ultimate gauge image's material
		}
		if (UltimateGaugeMID)
		{
			UltimateGaugeMID->SetScalarParameterValue(UltimatePercentParam, Percent);
		}
	}

	// gray out the ultimate icon until the gauge is full, full color once ready
	if (LiberationIcon)
	{
		LiberationIcon->SetColorAndOpacity(bReady ? FLinearColor::White : UltimateNotReadyTint);
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
