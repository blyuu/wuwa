#include "UI/LiberationCutinWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Engine/World.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void ULiberationCutinWidget::StartCutin(float InDuration, const FLinearColor& InAccentColor)
{
	Duration = FMath::Max(0.2f, InDuration);
	AccentColor = InAccentColor;
	StartGameTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	ForceVolatile(true);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> ULiberationCutinWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("CutinWidgetTree"));
	}
	if (!WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CutinRoot"));
	}
	return Super::RebuildWidget();
}

int32 ULiberationCutinWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements,
	int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	const FVector2D Size = Geometry.GetLocalSize();
	if (Size.X <= 1.f || Size.Y <= 1.f) return BaseLayer;

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : StartGameTime;
	float Progress = FMath::Clamp((Now - StartGameTime) / Duration, 0.f, 1.f);
#if WITH_EDITOR
	if (PreviewProgress >= 0.f) Progress = FMath::Clamp(PreviewProgress, 0.f, 1.f);
#endif
	const FVector2D Center(Size.X * .5f, Size.Y * .47f);
	const FPaintGeometry PaintGeometry = Geometry.ToPaintGeometry();
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const int32 EffectLayer = BaseLayer + 1;

	auto FillScreen = [&](const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(Elements, EffectLayer, PaintGeometry, WhiteBrush,
			ESlateDrawEffect::None, Color);
	};
	auto Stroke = [&](const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Thickness)
	{
		TArray<FVector2d> Points;
		Points.Add(FVector2d(A));
		Points.Add(FVector2d(B));
		FSlateDrawElement::MakeLines(Elements, EffectLayer + 1, PaintGeometry, Points,
			ESlateDrawEffect::None, Color, true, Thickness);
	};

	if (Progress < .19f)
	{
		// The first beat is an opaque black/white graphic cut before the close-up.
		FillScreen(FLinearColor::Black);
		for (int32 Index = 0; Index < 42; ++Index)
		{
			const float Angle = (Index / 42.f) * 2.f * PI + .17f;
			const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
			const float Inner = 35.f + (Index % 5) * 13.f;
			const float Outer = FMath::Max(Size.X, Size.Y) * (.57f + (Index % 4) * .08f);
			FLinearColor LineColor = FLinearColor::LerpUsingHSV(AccentColor, FLinearColor::White, .75f);
			LineColor.A = Index % 3 ? .62f : .95f;
			Stroke(Center + Direction * Inner, Center + Direction * Outer,
				LineColor, Index % 5 ? 1.8f : 4.5f);
		}
		Stroke(Center + FVector2D(-Size.X * .35f, Size.Y * .25f),
			Center + FVector2D(Size.X * .38f, -Size.Y * .26f), FLinearColor::White, 8.f);
	}
	else if (Progress < .78f)
	{
		// Keep the live character visible underneath the element-colored speed lines.
		FillScreen(FLinearColor(AccentColor.R * .08f, AccentColor.G * .08f, AccentColor.B * .08f, .28f));
		for (int32 Index = 0; Index < 30; ++Index)
		{
			const float Angle = (Index / 30.f) * 2.f * PI + .08f;
			const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
			const float Start = FMath::Min(Size.X, Size.Y) * (.24f + (Index % 3) * .05f);
			const float End = FMath::Max(Size.X, Size.Y) * .85f;
			FLinearColor LineColor = AccentColor;
			LineColor.A = Index % 4 ? .48f : .83f;
			Stroke(Center + Direction * Start, Center + Direction * End,
				LineColor, Index % 6 ? 1.5f : 3.8f);
		}
	}
	else
	{
		// Element-colored flash ends the overlay; the front camera remains through the montage.
		const float Flash = FMath::Clamp((1.f - Progress) / .22f, 0.f, 1.f);
		FLinearColor FlashColor = FLinearColor::LerpUsingHSV(AccentColor, FLinearColor::White, .6f);
		FlashColor.A = Flash * .88f;
		FillScreen(FlashColor);
		Stroke(FVector2D(0.f, Size.Y * .72f), FVector2D(Size.X, Size.Y * .26f),
			FLinearColor(1.f, 1.f, 1.f, Flash), 18.f);
		FLinearColor SlashColor = AccentColor;
		SlashColor.A = Flash;
		Stroke(FVector2D(0.f, Size.Y * .76f), FVector2D(Size.X, Size.Y * .30f),
			SlashColor, 38.f);
	}
	return EffectLayer + 2;
}
