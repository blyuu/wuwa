#pragma once

#include "GraphicsOptimizerTypes.h"

class UGameViewportClient;

class GRAPHICSOPTIMIZERRUNTIME_API FPerformanceCollector
{
public:
    void Begin(UGameViewportClient* InViewport);
    void Sample();
    FGraphicsPerformanceSummary Finish() const;

private:
    double PreviousTime = 0.0;
    TArray<double> FrameTimesMS;
    TArray<double> GameTimesMS;
    TArray<double> RenderTimesMS;
    TArray<double> RenderCriticalTimesMS;
    TArray<double> GPUTimesMS;
    TArray<double> StatFrameTimesMS;
    TArray<double> StatGameTimesMS;
    TArray<double> StatRenderTimesMS;
    TArray<double> StatGPUTimesMS;
    TWeakObjectPtr<UGameViewportClient> Viewport;
    int32 PreviousStatIndex = INDEX_NONE;
};
