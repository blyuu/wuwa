#include "GraphicsResolutionProbe.h"
#include "DynamicResolutionState.h"
#include "Engine/Engine.h"
#include "SceneView.h"
#include "UnrealClient.h"

FGraphicsResolutionProbe::FGraphicsResolutionProbe(const FAutoRegister& AutoRegister, FViewport* InViewport)
    : FSceneViewExtensionBase(AutoRegister), TargetViewport(InViewport)
{
}

bool FGraphicsResolutionProbe::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
    // The view-family callback checks the render target; some callers gather by scene.
    return TargetViewport && (!Context.Viewport || Context.Viewport == TargetViewport);
}

void FGraphicsResolutionProbe::PostRenderViewFamily_RenderThread(FRDGBuilder&, FSceneViewFamily& ViewFamily)
{
    if (!TargetViewport || ViewFamily.RenderTarget != TargetViewport ||
        !ViewFamily.EngineShowFlags.ScreenPercentage || !ViewFamily.GetScreenPercentageInterface() ||
        ViewFamily.Views.IsEmpty())
        return;

    const auto Fractions = ViewFamily.GetScreenPercentageInterface()->GetResolutionFractionsUpperBound();
    // UE's static viewport driver reports its selected primary fraction as this bound.
    // Dynamic resolution is rejected on the game thread before a value is used.
    const float SelectedPrimaryPercentage = Fractions[GDynamicPrimaryResolutionFraction] * 100.0f;
    if (!FMath::IsFinite(SelectedPrimaryPercentage) || SelectedPrimaryPercentage < 10.0f ||
        SelectedPrimaryPercentage > 100.0f)
        return;
    LatestRenderedPercentage.Store(SelectedPrimaryPercentage);
    LatestSampleSerial.Store(LatestSampleSerial.Load() + 1);
}

bool FGraphicsResolutionProbe::GetStablePrimaryPercentage(float& OutPercentage)
{
    FDynamicResolutionStateInfos DynamicInfo;
    if (GEngine)
    {
        GEngine->GetDynamicResolutionCurrentStateInfos(DynamicInfo);
        if (DynamicInfo.Status == EDynamicResolutionStatus::Enabled ||
            DynamicInfo.Status == EDynamicResolutionStatus::DebugForceEnabled)
        {
            RecentPercentages.Reset();
            LastCollectedSerial = LatestSampleSerial.Load();
            StalePolls = 0;
            return false;
        }
    }
    const FIntPoint ViewportSize = TargetViewport ? TargetViewport->GetSizeXY() : FIntPoint::ZeroValue;
    if (ViewportSize.X <= 0 || ViewportSize.Y <= 0) return false;
    if (ViewportSize != LastViewportSize)
    {
        RecentPercentages.Reset();
        LastViewportSize = ViewportSize;
    }
    const uint32 Serial = LatestSampleSerial.Load();
    if (Serial != LastCollectedSerial)
    {
        LastCollectedSerial = Serial;
        StalePolls = 0;
        RecentPercentages.Add(LatestRenderedPercentage.Load());
        if (RecentPercentages.Num() > 8) RecentPercentages.RemoveAt(0);
    }
    else if (++StalePolls > 8)
    {
        RecentPercentages.Reset();
        return false;
    }
    if (RecentPercentages.Num() < 3) return false;
    float Minimum = RecentPercentages[0];
    float Maximum = Minimum;
    double Sum = 0.0;
    for (float Value : RecentPercentages)
    {
        Minimum = FMath::Min(Minimum, Value);
        Maximum = FMath::Max(Maximum, Value);
        Sum += Value;
    }
    if (Maximum - Minimum > 0.5f) return false;
    OutPercentage = static_cast<float>(Sum / RecentPercentages.Num());
    return true;
}
