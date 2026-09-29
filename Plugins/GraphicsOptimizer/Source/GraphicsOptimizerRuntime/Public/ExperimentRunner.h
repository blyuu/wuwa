#pragma once

#include "BenchmarkManager.h"
#include "GraphicsSettingManager.h"
#include "OptimizationManager.h"

class UGameViewportClient;

class GRAPHICSOPTIMIZERRUNTIME_API FExperimentRunner
{
public:
    bool Start(const FGraphicsBenchmarkOptions& InOptions, UGameViewportClient* InViewport);
    void Tick(float ObservedPrimaryResolutionScale);
    void Cancel();
    void Abort(const FString& Reason);
    EGraphicsOptimizerState GetState() const { return State; }
    const TArray<FGraphicsExperimentResult>& GetResults() const { return Results; }
    const FString& GetLastError() const { return LastError; }
    const FGraphicsBenchmarkOptions& GetOptions() const { return Options; }
    float GetFrameRateCapFPS() const { return FrameRateCapFPS; }
    float GetObservedBaselineResolutionScale() const { return Options.ObservedBaselineResolutionScale; }
    int32 GetBestResultIndex() const { return Optimization.GetBestResultIndex(); }
    int32 GetFastestResultIndex() const { return Optimization.GetFastestResultIndex(); }
    float GetMinimumUsefulGainFPS() const { return Options.MinimumUsefulGainFPS; }
    bool HasMetOptimizationTarget() const { return Optimization.HasMetTarget(); }
    bool HadTargetBeforeValidation() const { return Optimization.HadTargetBeforeValidation(); }
    bool IsValidationComplete() const { return Optimization.IsValidationComplete(); }
    const TArray<FGraphicsValidationResult>& GetValidationResults() const { return Optimization.GetValidationResults(); }
    const FGraphicsValidationResult* GetSelectedValidation() const { return Optimization.GetSelectedValidation(); }
    bool WasOptimizationRequested() const { return Options.bRunOptimization; }
    bool IsRunning() const;

private:
    void SetState(EGraphicsOptimizerState Next);
    void BuildCandidates();
    void AddResolutionCandidate(float BaselineResolution);
    bool CompleteMeasurement(bool bBaseline);
    void AdvanceExperiment();
    void AdvanceOptimization();
    void FinishRun(bool bFailed);

    EGraphicsOptimizerState State = EGraphicsOptimizerState::Idle;
    FGraphicsBenchmarkOptions Options;
    FGraphicsSettingManager Settings;
    FBenchmarkManager Benchmark;
    FOptimizationManager Optimization;
    TArray<FGraphicsExperimentCandidate> Candidates;
    TArray<FGraphicsExperimentResult> Results;
    Scalability::FQualityLevels AppliedConfiguration;
    int32 CandidateIndex = 0;
    bool bMeasuringOptimization = false;
    bool bMeasuringValidation = false;
    FString ActiveOptimizationLabel;
    float CurrentObservedPrimaryResolutionScale = 0.0f;
    float FrameRateCapFPS = 0.0f;
    TWeakObjectPtr<UGameViewportClient> Viewport;
    FString LastError;
};
