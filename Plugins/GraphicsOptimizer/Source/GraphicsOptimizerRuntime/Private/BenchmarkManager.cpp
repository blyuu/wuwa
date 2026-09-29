#include "BenchmarkManager.h"
#include "HAL/PlatformTime.h"

void FBenchmarkManager::BeginWarmup(float DurationSeconds)
{
    EndTime = FPlatformTime::Seconds() + DurationSeconds;
}

void FBenchmarkManager::BeginMeasurement(float DurationSeconds, UGameViewportClient* Viewport)
{
    Collector.Begin(Viewport);
    EndTime = FPlatformTime::Seconds() + DurationSeconds;
}

void FBenchmarkManager::TickMeasurement()
{
    Collector.Sample();
}

bool FBenchmarkManager::IsPeriodComplete() const
{
    return FPlatformTime::Seconds() >= EndTime;
}

FGraphicsPerformanceSummary FBenchmarkManager::FinishMeasurement() const
{
    return Collector.Finish();
}
