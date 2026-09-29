#pragma once

#include "GraphicsOptimizerTypes.h"

struct FGraphicsValidationResult
{
    int32 SourceResultIndex = INDEX_NONE;
    TArray<int32> RunResultIndices;
    double MinimumFPS = 0.0;
    double MaximumFPS = 0.0;
    double MedianFPS = 0.0;
    int32 TargetMetRuns = 0;
    bool bAllTargetEvaluationsValid = false;
    bool bStableTargetMet = false;
};

// Deterministic, measured search. No prediction or permanent setting changes.
class GRAPHICSOPTIMIZERRUNTIME_API FOptimizationManager
{
public:
    void Reset();
    void Initialize(const TArray<FGraphicsExperimentResult>& Results, float MinimumUsefulGainFPS);
    bool NextConfiguration(const TArray<FGraphicsExperimentResult>& Results,
        Scalability::FQualityLevels& OutConfiguration, FString& OutLabel);
    void RecordResult(const TArray<FGraphicsExperimentResult>& Results, int32 ResultIndex);
    void BeginValidation(const TArray<FGraphicsExperimentResult>& Results, float TargetFPS);
    bool NextValidationConfiguration(const TArray<FGraphicsExperimentResult>& Results,
        Scalability::FQualityLevels& OutConfiguration, FString& OutLabel);
    void RecordValidationResult(const TArray<FGraphicsExperimentResult>& Results, int32 ResultIndex);
    void FinalizeValidation(const TArray<FGraphicsExperimentResult>& Results);
    int32 GetBestResultIndex() const { return BestResultIndex; }
    int32 GetFastestResultIndex() const { return FastestResultIndex; }
    bool HasMetTarget() const { return bValidationComplete ? bValidationTargetMet : bMetTarget; }
    bool HadTargetBeforeValidation() const { return bMetTarget; }
    bool IsRecoveringQuality() const { return bRecoveringQuality; }
    const TArray<FGraphicsValidationResult>& GetValidationResults() const { return ValidationResults; }
    const FGraphicsValidationResult* GetSelectedValidation() const;
    bool IsValidationComplete() const { return bValidationComplete; }
    static constexpr int32 ValidationRunsPerCandidate = 3;

private:
    TArray<int32> RankedSingleResultIndices;
    int32 NextRankedIndex = 0;
    int32 BestResultIndex = INDEX_NONE;
    int32 FastestResultIndex = INDEX_NONE;
    int32 NextRecoverySettingIndex = 0;
    float MinimumUsefulGainFPS = 0.75f;
    bool bMetTarget = false;
    bool bRecoveringQuality = false;
    TArray<FGraphicsValidationResult> ValidationResults;
    int32 CurrentValidationIndex = 0;
    bool bValidationComplete = false;
    bool bValidationTargetMet = false;
};
