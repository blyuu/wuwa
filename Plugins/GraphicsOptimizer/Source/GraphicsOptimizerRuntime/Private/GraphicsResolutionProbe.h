#pragma once

#include "SceneViewExtension.h"

class FViewport;

// Observes the actual primary resolution fraction chosen for the PIE viewport.
// Dynamic resolution is excluded because a single baseline percentage is not stable.
class FGraphicsResolutionProbe final : public FSceneViewExtensionBase
{
public:
    FGraphicsResolutionProbe(const FAutoRegister& AutoRegister, FViewport* InViewport);
    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
    virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
    virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;

    FViewport* GetViewport() const { return TargetViewport; }
    bool GetStablePrimaryPercentage(float& OutPercentage);

protected:
    virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

private:
    FViewport* TargetViewport = nullptr;
    FIntPoint LastViewportSize = FIntPoint::ZeroValue;
    TArray<float> RecentPercentages;
    TAtomic<float> LatestRenderedPercentage{0.0f};
    TAtomic<uint32> LatestSampleSerial{0};
    uint32 LastCollectedSerial = 0;
    int32 StalePolls = 0;
};
