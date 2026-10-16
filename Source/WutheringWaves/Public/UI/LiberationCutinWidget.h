#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LiberationCutinWidget.generated.h"

/** Short, resolution-independent screen graphic over the liberation camera cut. */
UCLASS()
class WUTHERINGWAVES_API ULiberationCutinWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void StartCutin(float InDuration, const FLinearColor& InAccentColor = FLinearColor(.25f, .72f, 1.f));
#if WITH_EDITOR
	void SetPreviewProgress(float InProgress) { PreviewProgress = InProgress; }
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	float StartGameTime = 0.f;
	float Duration = 1.6f;
	FLinearColor AccentColor = FLinearColor(.25f, .72f, 1.f);
#if WITH_EDITOR
	float PreviewProgress = -1.f;
#endif
};
