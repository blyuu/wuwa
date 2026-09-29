#pragma once

#include "CoreMinimal.h"
#include "Scalability.h"

enum class EGraphicsOptimizerState : uint8
{
    Idle, Preparing, WarmupBaseline, MeasuringBaseline, ApplyingConfiguration,
    WarmupExperiment, MeasuringExperiment, ProcessingResult, Restoring, Completed, Failed
};

enum class EGraphicsSetting : uint8
{
    ResolutionScale, Shadow, GlobalIllumination, Reflection, PostProcess, Texture, Effects
};

struct FGraphicsExperimentCandidate
{
    EGraphicsSetting Setting = EGraphicsSetting::Shadow;
    float Value = 0.0f;
    FString Label;
};

struct FGraphicsBenchmarkOptions
{
    float TargetFPS = 60.0f;
    float WarmupSeconds = 1.0f;
    float MeasurementSeconds = 5.0f;
    int32 MinimumSamples = 30;
    bool bFreezeGameplay = false;
    bool bRunOptimization = false;
    float ObservedBaselineResolutionScale = 0.0f; // 0 means no stable PIE viewport observation.
    float MinimumUsefulGainFPS = 0.75f;
};

struct FGraphicsPerformanceSummary
{
    int32 FrameSamples = 0;
    int32 GameSamples = 0;
    int32 RenderSamples = 0;
    int32 RenderCriticalSamples = 0;
    int32 GPUSamples = 0;
    int32 LongHitchSamples = 0;
    int32 StatUnitSamples = 0;
    double AverageFPS = 0.0;
    double AverageFrameMS = 0.0;
    double LongestFrameMS = 0.0;
    double P99FrameMS = 0.0;
    double OnePercentLowFPS = 0.0;
    double AverageGameMS = 0.0;
    double AverageRenderMS = 0.0;
    double AverageRenderCriticalMS = 0.0;
    double AverageGPUMS = 0.0;
    double StatUnitFrameMS = 0.0;
    double StatUnitGameMS = 0.0;
    double StatUnitRenderMS = 0.0;
    double StatUnitGPUMS = 0.0;
};

struct FGraphicsExperimentResult
{
    int32 ExperimentNumber = 0; // Zero is baseline.
    FString Label;
    FGraphicsExperimentCandidate Candidate;
    Scalability::FQualityLevels AppliedConfiguration;
    FGraphicsPerformanceSummary Performance;
    double FPSDifference = 0.0;
    double GPUTimeDifferenceMS = 0.0;
    float FrameRateCapFPS = 0.0f;
    float ObservedPrimaryResolutionScale = 0.0f;
    bool bFPSComparisonLimited = false;
    bool bTargetEvaluationValid = true;
    bool bGPUComparisonAvailable = false;
    bool bMetTargetFPS = false;
    bool bOptimizationStep = false;
    bool bQualityRecoveryStep = false;
    bool bValidationStep = false;
};
