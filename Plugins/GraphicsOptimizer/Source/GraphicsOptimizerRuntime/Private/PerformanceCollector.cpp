#include "PerformanceCollector.h"
#include "HAL/PlatformTime.h"
#include "RenderTimer.h"
#include "RHIGlobals.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"

namespace
{
double Average(const TArray<double>& Values)
{
    double Total = 0.0;
    for (const double Value : Values) Total += Value;
    return Values.IsEmpty() ? 0.0 : Total / Values.Num();
}
}

void FPerformanceCollector::Begin(UGameViewportClient* InViewport)
{
    FrameTimesMS.Reset();
    GameTimesMS.Reset();
    RenderTimesMS.Reset();
    RenderCriticalTimesMS.Reset();
    GPUTimesMS.Reset();
    StatFrameTimesMS.Reset();
    StatGameTimesMS.Reset();
    StatRenderTimesMS.Reset();
    StatGPUTimesMS.Reset();
    Viewport = InViewport;
    PreviousStatIndex = INDEX_NONE;
#if !UE_BUILD_SHIPPING
    if (InViewport && InViewport->GetStatUnitData()) PreviousStatIndex = InViewport->GetStatUnitData()->CurrentIndex;
#endif
    PreviousTime = FPlatformTime::Seconds();
}

void FPerformanceCollector::Sample()
{
    const double Now = FPlatformTime::Seconds();
    const double FrameMS = (Now - PreviousTime) * 1000.0;
    PreviousTime = Now;
    if (FrameMS <= 0.0) return;
    FrameTimesMS.Add(FrameMS);
    if (GGameThreadTime > 0) GameTimesMS.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
    if (GRenderThreadTime > 0) RenderTimesMS.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));
    if (GRenderThreadTimeCriticalPath > 0) RenderCriticalTimesMS.Add(FPlatformTime::ToMilliseconds(GRenderThreadTimeCriticalPath));
    if (GGPUFrameTime > 0) GPUTimesMS.Add(FPlatformTime::ToMilliseconds(GGPUFrameTime));
#if !UE_BUILD_SHIPPING
    if (UGameViewportClient* GameViewport = Viewport.Get())
    {
        if (GameViewport->IsStatEnabled(TEXT("Unit")))
        {
            if (const FStatUnitData* Unit = GameViewport->GetStatUnitData())
            {
                if (Unit->CurrentIndex != PreviousStatIndex)
                {
                    PreviousStatIndex = Unit->CurrentIndex;
                    if (Unit->RawFrameTime > 0.0f) StatFrameTimesMS.Add(Unit->RawFrameTime);
                    if (Unit->RawGameThreadTime > 0.0f) StatGameTimesMS.Add(Unit->RawGameThreadTime);
                    if (Unit->RawRenderThreadTime > 0.0f) StatRenderTimesMS.Add(Unit->RawRenderThreadTime);
                    if (Unit->RawGPUFrameTime[0] > 0.0f) StatGPUTimesMS.Add(Unit->RawGPUFrameTime[0]);
                }
            }
        }
    }
#endif
}

FGraphicsPerformanceSummary FPerformanceCollector::Finish() const
{
    FGraphicsPerformanceSummary Result;
    Result.FrameSamples = FrameTimesMS.Num();
    Result.GameSamples = GameTimesMS.Num();
    Result.RenderSamples = RenderTimesMS.Num();
    Result.RenderCriticalSamples = RenderCriticalTimesMS.Num();
    Result.GPUSamples = GPUTimesMS.Num();
    Result.StatUnitSamples = StatFrameTimesMS.Num();
    Result.AverageFrameMS = Average(FrameTimesMS);
    for (const double FrameMS : FrameTimesMS)
    {
        Result.LongestFrameMS = FMath::Max(Result.LongestFrameMS, FrameMS);
        if (FrameMS >= 250.0) ++Result.LongHitchSamples;
    }
    Result.AverageFPS = Result.AverageFrameMS > 0.0 ? 1000.0 / Result.AverageFrameMS : 0.0;
    Result.AverageGameMS = Average(GameTimesMS);
    Result.AverageRenderMS = Average(RenderTimesMS);
    Result.AverageRenderCriticalMS = Average(RenderCriticalTimesMS);
    Result.AverageGPUMS = Average(GPUTimesMS);
    Result.StatUnitFrameMS = Average(StatFrameTimesMS);
    Result.StatUnitGameMS = Average(StatGameTimesMS);
    Result.StatUnitRenderMS = Average(StatRenderTimesMS);
    Result.StatUnitGPUMS = Average(StatGPUTimesMS);
    if (!FrameTimesMS.IsEmpty())
    {
        TArray<double> Sorted = FrameTimesMS;
        Sorted.Sort();
        const int32 P99Index = FMath::Clamp(FMath::CeilToInt(Sorted.Num() * 0.99) - 1, 0, Sorted.Num() - 1);
        Result.P99FrameMS = Sorted[P99Index];
        const int32 SlowCount = FMath::Max(1, FMath::CeilToInt(Sorted.Num() * 0.01));
        double SlowTotal = 0.0;
        for (int32 Index = Sorted.Num() - SlowCount; Index < Sorted.Num(); ++Index) SlowTotal += Sorted[Index];
        Result.OnePercentLowFPS = SlowTotal > 0.0 ? 1000.0 / (SlowTotal / SlowCount) : 0.0;
    }
    return Result;
}
