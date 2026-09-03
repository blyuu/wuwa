// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/DamageNumberWidget.h"

#include "Components/TextBlock.h"

void UDamageNumberWidget::SetDamage(float Amount, const FGameplayTag& ElementTag)
{
	if (!DamageText)
	{
		return;
	}

	DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Amount)));

	// element -> color (falls back to white if the element isn't mapped in the WBP)
	const FLinearColor* Found = ElementColors.Find(ElementTag);
	DamageText->SetColorAndOpacity(FSlateColor(Found ? *Found : FLinearColor::White));
}

void UDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	Elapsed += InDeltaTime;
	const float Alpha = (LifeSeconds > 0.f) ? FMath::Clamp(Elapsed / LifeSeconds, 0.f, 1.f) : 1.f;

	// rise (translate up in screen space) + fade out over the lifetime
	SetRenderTranslation(FVector2D(0.f, -RiseDistance * Alpha));
	SetRenderOpacity(1.f - Alpha);

	if (Elapsed >= LifeSeconds)
	{
		RemoveFromParent();
	}
}
