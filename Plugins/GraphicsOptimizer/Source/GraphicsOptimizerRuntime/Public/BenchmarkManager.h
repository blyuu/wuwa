#pragma once

#include "PerformanceCollector.h"

class GRAPHICSOPTIMIZERRUNTIME_API FBenchmarkManager
{
public:
    void BeginWarmup(float DurationSeconds);
    void BeginMeasurement(float DurationSeconds, UGameViewportClient* Viewport);
    void TickMeasurement();
    bool IsPeriodComplete() const;
    FGraphicsPerformanceSummary FinishMeasurement() const;

private:
    double EndTime = 0.0;
    FPerformanceCollector Collector;
};
