// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WuwaOverlayWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "DataAsset/CharacterDataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	constexpr int32 CircuitGroups = 5;
	constexpr int32 CircuitTicksPerGroup = 9;
	constexpr float CircuitArtWidth = 1200.f;
	constexpr float CircuitArtHeight = 148.f;
	FLinearColor DefaultCircuitColor(const FGameplayTag& Element)
	{
		if (Element == ElementTags::Character_Element_Fusion) return FLinearColor(1.f, .32f, .08f);
		if (Element == ElementTags::Character_Element_Glacio) return FLinearColor(.25f, .78f, 1.f);
		if (Element == ElementTags::Character_Element_Aero) return FLinearColor(.16f, .92f, .65f);
		if (Element == ElementTags::Character_Element_Electro) return FLinearColor(.66f, .32f, 1.f);
		if (Element == ElementTags::Character_Element_Spectro) return FLinearColor(1.f, .82f, .27f);
		if (Element == ElementTags::Character_Element_Havoc) return FLinearColor(.85f, .18f, .5f);
		return FLinearColor::White;
	}
}

void UWuwaOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!ResonanceCircuitImage)
	{
		return;
	}
	if (UTexture2D* CircuitTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/WutheringWaves/UI/T_ResonanceCircuit_Empty.T_ResonanceCircuit_Empty")))
	{
		ResonanceCircuitImage->SetBrushFromTexture(CircuitTexture, false);
		// Keep its layout geometry, but paint the neutral art and colored ticks together in NativePaint.
		ResonanceCircuitImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
		ResonanceCircuitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	// Position, anchors and size belong to WBP_Overlay; do not override its designer layout here.
}

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
	CircuitSpectrumFade = 0.f;
	CircuitSpectrumTime = 0.f;

	BoundASC = InASC;
	if (!BoundASC)
	{
		CircuitPercent = 0.f;
		Invalidate(EInvalidateWidgetReason::Paint);
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

	// One neutral circuit texture for every character; the current element only colors filled ticks.
	if (ResonanceCircuitImage)
	{
		UTexture2D* CircuitTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/WutheringWaves/UI/T_ResonanceCircuit_Empty.T_ResonanceCircuit_Empty"));
		if (CircuitTexture)
		{
			ResonanceCircuitImage->SetBrushFromTexture(CircuitTexture, false);
			ResonanceCircuitImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
			ResonanceCircuitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	// tint the gauges (variation donut + ultimate) to match the character's element
	ApplyElementColor(Data->ElementTag);
}

void UWuwaOverlayWidget::ApplyElementColor(const FGameplayTag& ElementTag)
{
	// no mapping -> fall back to white (the material's default)
	const FLinearColor* Found = ElementColors.Find(ElementTag);
	const FLinearColor Color = Found ? *Found : DefaultCircuitColor(ElementTag);
	CircuitColor = Color;
	Invalidate(EInvalidateWidgetReason::Paint);

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
	if (LiberationCooldownRim)
	{
		LiberationCooldownRim->SetColorAndOpacity(Color);
	}
}

void UWuwaOverlayWidget::NativeDestruct()
{
	UnbindFromCurrentASC();
	Super::NativeDestruct();
}

int32 UWuwaOverlayWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!ResonanceCircuitImage || !ResonanceCircuitImage->IsVisible())
	{
		return BaseLayer;
	}

	const FGeometry ImageGeometry = ResonanceCircuitImage->GetCachedGeometry();
	const FVector2D ImageSize = ImageGeometry.GetLocalSize();
	if (ImageSize.X <= 0.f || ImageSize.Y <= 0.f)
	{
		return BaseLayer;
	}
	const FVector2D Origin = AllottedGeometry.AbsoluteToLocal(ImageGeometry.LocalToAbsolute(FVector2D::ZeroVector));
	const FVector2D Scale(ImageSize.X / CircuitArtWidth, ImageSize.Y / CircuitArtHeight);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	// A quiet, deterministic audio-spectrum-like pulse. Both halves grow away from the circuit's
	// center line; independent frequencies keep the bars from moving as one large wave.
	if (CircuitSpectrumFade > KINDA_SMALL_NUMBER && CircuitSpectrumStrength > 0.f)
	{
		for (int32 Tick = 0; Tick < CircuitGroups * CircuitTicksPerGroup; ++Tick)
		{
			const int32 Group = Tick / CircuitTicksPerGroup;
			const int32 WithinGroup = Tick % CircuitTicksPerGroup;
			const float X = 48.f + Group * 222.f + WithinGroup * 20.f + 4.5f;
			const float Fast = FMath::Abs(FMath::Sin(CircuitSpectrumTime * (2.1f + (Tick % 7) * 0.13f) + Tick * 0.71f));
			const float Slow = FMath::Abs(FMath::Sin(CircuitSpectrumTime * 0.73f + Tick * 0.29f));
			const float Height = 30.f + 48.f * (0.16f + 0.84f * Fast * (0.4f + 0.6f * Slow));
			const float Alpha = FMath::Clamp(CircuitSpectrumStrength * CircuitSpectrumFade *
				(0.5f + 0.5f * Fast), 0.f, 1.f);
			const FVector2D GlowPos = Origin + FVector2D((X - 6.f) * Scale.X, (82.f - Height * 0.5f) * Scale.Y);
			const FVector2D GlowSize(12.f * Scale.X, Height * Scale.Y);
			FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 1,
				AllottedGeometry.ToPaintGeometry(GlowSize, FSlateLayoutTransform(GlowPos)),
				WhiteBrush, ESlateDrawEffect::None,
				FLinearColor(CircuitColor.R, CircuitColor.G, CircuitColor.B, Alpha * 0.28f));
			const FVector2D BarPos = Origin + FVector2D((X - 2.f) * Scale.X, (82.f - Height * 0.5f) * Scale.Y);
			const FVector2D BarSize(4.f * Scale.X, Height * Scale.Y);
			FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 1,
				AllottedGeometry.ToPaintGeometry(BarSize, FSlateLayoutTransform(BarPos)),
				WhiteBrush, ESlateDrawEffect::None,
				FLinearColor(CircuitColor.R, CircuitColor.G, CircuitColor.B, Alpha));
		}
	}
	// The gray track and the colored ticks must use one paint geometry. Drawing the UImage separately
	// could place its brush in another row when the WBP's canvas and render transforms differ.
	FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 2,
		AllottedGeometry.ToPaintGeometry(ImageSize, FSlateLayoutTransform(Origin)),
		&ResonanceCircuitImage->GetBrush(), ESlateDrawEffect::None, FLinearColor::White);
	if (CircuitPercent <= 0.f)
	{
		return BaseLayer + 2;
	}
	const int32 LitTicks = FMath::Clamp(FMath::CeilToInt(CircuitPercent * CircuitGroups * CircuitTicksPerGroup),
		0, CircuitGroups * CircuitTicksPerGroup);
	const FLinearColor GlowColor(CircuitColor.R, CircuitColor.G, CircuitColor.B, 0.23f);
	const FLinearColor TickColor = FLinearColor::LerpUsingHSV(CircuitColor, FLinearColor::White, 0.28f);
	for (int32 Tick = 0; Tick < LitTicks; ++Tick)
	{
		const int32 Group = Tick / CircuitTicksPerGroup;
		const int32 WithinGroup = Tick % CircuitTicksPerGroup;
		const float X = 48.f + Group * 222.f + WithinGroup * 20.f;
		const FVector2D GlowPos = Origin + FVector2D((X - 3.f) * Scale.X, 58.f * Scale.Y);
		const FVector2D GlowSize(15.f * Scale.X, 38.f * Scale.Y);
		FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 3,
			AllottedGeometry.ToPaintGeometry(GlowSize, FSlateLayoutTransform(GlowPos)),
			WhiteBrush, ESlateDrawEffect::None, GlowColor);
		const FVector2D TickPos = Origin + FVector2D((X + 1.f) * Scale.X, 64.f * Scale.Y);
		const FVector2D TickSize(7.f * Scale.X, 23.f * Scale.Y);
		FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 4,
			AllottedGeometry.ToPaintGeometry(TickSize, FSlateLayoutTransform(TickPos)),
			WhiteBrush, ESlateDrawEffect::None, TickColor);
	}
	return BaseLayer + 4;
}

void UWuwaOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float SpectrumTarget = CircuitPercent >= 1.f - KINDA_SMALL_NUMBER ? 1.f : 0.f;
	const float PreviousSpectrumFade = CircuitSpectrumFade;
	const float FadeSpeed = SpectrumTarget > CircuitSpectrumFade ? 3.5f : 2.5f;
	CircuitSpectrumFade = FMath::FInterpConstantTo(CircuitSpectrumFade, SpectrumTarget, InDeltaTime, FadeSpeed);
	if (SpectrumTarget > 0.f || PreviousSpectrumFade > KINDA_SMALL_NUMBER || CircuitSpectrumFade > KINDA_SMALL_NUMBER)
	{
		CircuitSpectrumTime += InDeltaTime * CircuitSpectrumSpeed;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	// cooldowns are time-based, so poll them each frame (attribute delegates don't fire per-tick)
	UpdateCooldown(ResonanceSkillCooldown, ResonanceSkillCooldownText, SkillCooldownMID, CooldownTags::Cooldown_ResonanceSkill);
	UpdateCooldown(LiberationCooldown, LiberationCooldownText, LiberationCooldownMID, CooldownTags::Cooldown_Liberation);
	if (LiberationCooldownRim)
	{
		const bool bLiberationCoolingDown = LiberationCooldown
			&& LiberationCooldown->GetVisibility() == ESlateVisibility::HitTestInvisible;
		LiberationCooldownRim->SetVisibility(bLiberationCoolingDown
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// buff icon above the HP bar: shown while the active character's attack power is boosted (수수's team buff)
	if (BuffIcon)
	{
		const bool bBuffed = BoundASC
			&& BoundASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetAttackPowerAttribute()) > 1.001f;
		BuffIcon->SetVisibility(bBuffed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
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

	// "cur / max" text inside the bar
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(
			FString::Printf(TEXT("%d/%d"), FMath::RoundToInt(Hp), FMath::RoundToInt(MaxHp))));
	}

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
	CircuitPercent = MaxEnergy > 0.f ? FMath::Clamp(Energy / MaxEnergy, 0.f, 1.f) : 0.f;
	Invalidate(EInvalidateWidgetReason::Paint);
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
