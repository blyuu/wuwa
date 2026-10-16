#include "OptimizationManager.h"
#include "GraphicsSettingManager.h"
#include "GraphicsOptimizerLocalization.h"

namespace
{
// Fixed retry order; no image-quality score is available in this phase.
constexpr EGraphicsSetting RecoveryOrder[] = {
    EGraphicsSetting::ResolutionScale, EGraphicsSetting::GlobalIllumination,
    EGraphicsSetting::Shadow, EGraphicsSetting::Reflection,
    EGraphicsSetting::PostProcess, EGraphicsSetting::Texture, EGraphicsSetting::Effects
};

int32 CountChangedSettings(const Scalability::FQualityLevels& Original, const Scalability::FQualityLevels& Candidate)
{
    int32 Count = 0;
    for (EGraphicsSetting Setting : RecoveryOrder)
    {
        if (!FMath::IsNearlyEqual(FGraphicsSettingManager::GetValue(Original, Setting),
            FGraphicsSettingManager::GetValue(Candidate, Setting), 0.1f))
            ++Count;
    }
    return Count;
}
}

void FOptimizationManager::Reset()
{
    RankedSingleResultIndices.Reset();
    NextRankedIndex = 0;
    NextRecoverySettingIndex = 0;
    BestResultIndex = INDEX_NONE;
    FastestResultIndex = INDEX_NONE;
    bMetTarget = false;
    bRecoveringQuality = false;
    ValidationResults.Reset();
    CurrentValidationIndex = 0;
    bValidationComplete = false;
    bValidationTargetMet = false;
}

void FOptimizationManager::Initialize(const TArray<FGraphicsExperimentResult>& Results, float InMinimumUsefulGainFPS)
{
    Reset();
    MinimumUsefulGainFPS = FMath::Max(0.0f, InMinimumUsefulGainFPS);
    if (Results.IsEmpty()) return;

    BestResultIndex = 0;
    FastestResultIndex = 0;
    bMetTarget = Results[0].bMetTargetFPS;
    const bool bBaselineCapLimited = !Results[0].bTargetEvaluationValid && Results[0].bFPSComparisonLimited;
    const double BaselineFPS = Results[0].Performance.AverageFPS;
    for (int32 Index = 1; Index < Results.Num(); ++Index)
    {
        const FGraphicsExperimentResult& Result = Results[Index];
        if (Result.Performance.AverageFPS > Results[FastestResultIndex].Performance.AverageFPS)
            FastestResultIndex = Index;
        if (bBaselineCapLimited) continue;
        if (Result.bMetTargetFPS)
        {
            // A single quality reduction costs less than any combined reduction.
            if (!bMetTarget || (BestResultIndex != 0 &&
                Result.Performance.AverageFPS > Results[BestResultIndex].Performance.AverageFPS))
            {
                BestResultIndex = Index;
            }
            bMetTarget = true;
        }
        else if (!bMetTarget &&
            (BestResultIndex == 0
                ? Result.Performance.AverageFPS >= BaselineFPS + MinimumUsefulGainFPS
                : Result.Performance.AverageFPS > Results[BestResultIndex].Performance.AverageFPS))
        {
            BestResultIndex = Index;
        }
        if (Result.Performance.AverageFPS > BaselineFPS + 0.5)
        {
            RankedSingleResultIndices.Add(Index);
        }
    }
    RankedSingleResultIndices.Sort([&Results](int32 A, int32 B)
    {
        return Results[A].Performance.AverageFPS > Results[B].Performance.AverageFPS;
    });
}

bool FOptimizationManager::NextConfiguration(const TArray<FGraphicsExperimentResult>& Results,
    Scalability::FQualityLevels& OutConfiguration, FString& OutLabel)
{
    if (!Results.IsValidIndex(BestResultIndex)) return false;
    if (bMetTarget)
    {
        bRecoveringQuality = true;
        const Scalability::FQualityLevels& Original = Results[0].AppliedConfiguration;
        const Scalability::FQualityLevels& Best = Results[BestResultIndex].AppliedConfiguration;
        while (NextRecoverySettingIndex < UE_ARRAY_COUNT(RecoveryOrder))
        {
            const EGraphicsSetting Setting = RecoveryOrder[NextRecoverySettingIndex++];
            const float OriginalValue = FGraphicsSettingManager::GetValue(Original, Setting);
            if (FMath::IsNearlyEqual(FGraphicsSettingManager::GetValue(Best, Setting), OriginalValue, 0.1f))
                continue;
            OutConfiguration = Best;
            FGraphicsSettingManager::SetValue(OutConfiguration, Setting, OriginalValue);
            const FString ValueLabel = Setting == EGraphicsSetting::ResolutionScale && OriginalValue == 0.0f
                ? FString(GOTEXT("project default")) : FString::Printf(TEXT("%.0f"), OriginalValue);
            OutLabel = FGraphicsOptimizerLocalization::Printf(TEXT("Quality recovery: restore %s to %s"),
                GOTR(FGraphicsSettingManager::GetSettingName(Setting)), *ValueLabel);
            return true;
        }
        return false;
    }
    if (BestResultIndex == 0) return false;
    // At the frame cap, further quality reductions cannot establish whether the target is met.
    if (Results.IsValidIndex(FastestResultIndex) &&
        !Results[FastestResultIndex].bTargetEvaluationValid && Results[FastestResultIndex].bFPSComparisonLimited)
        return false;
    while (NextRankedIndex < RankedSingleResultIndices.Num())
    {
        const FGraphicsExperimentResult& Single = Results[RankedSingleResultIndices[NextRankedIndex++]];
        const FGraphicsExperimentCandidate& Candidate = Single.Candidate;
        const Scalability::FQualityLevels& Best = Results[BestResultIndex].AppliedConfiguration;
        if (FMath::IsNearlyEqual(FGraphicsSettingManager::GetValue(Best, Candidate.Setting), Candidate.Value, 0.1f))
            continue;
        OutConfiguration = Best;
        FGraphicsSettingManager::SetValue(OutConfiguration, Candidate.Setting, Candidate.Value);
        FString BaseLabel = Results[BestResultIndex].Label;
        BaseLabel.RemoveFromStart(GOTEXT("Combined: "));
        OutLabel = FGraphicsOptimizerLocalization::Printf(TEXT("Combined: %s + %s"), *BaseLabel, *Candidate.Label);
        return true;
    }
    return false;
}

void FOptimizationManager::RecordResult(const TArray<FGraphicsExperimentResult>& Results, int32 ResultIndex)
{
    if (!Results.IsValidIndex(ResultIndex) || !Results.IsValidIndex(BestResultIndex)) return;
    const FGraphicsExperimentResult& Result = Results[ResultIndex];
    if (!Results.IsValidIndex(FastestResultIndex) ||
        Result.Performance.AverageFPS > Results[FastestResultIndex].Performance.AverageFPS)
        FastestResultIndex = ResultIndex;
    if (bRecoveringQuality)
    {
        if (Result.bMetTargetFPS)
        {
            BestResultIndex = ResultIndex;
            NextRecoverySettingIndex = 0;
        }
        return;
    }
    if (Result.bMetTargetFPS ||
        Result.Performance.AverageFPS >= Results[BestResultIndex].Performance.AverageFPS + MinimumUsefulGainFPS)
    {
        BestResultIndex = ResultIndex;
        bMetTarget = Result.bMetTargetFPS;
    }
}

void FOptimizationManager::BeginValidation(const TArray<FGraphicsExperimentResult>& Results, float TargetFPS)
{
    ValidationResults.Reset();
    CurrentValidationIndex = 0;
    bValidationComplete = false;
    bValidationTargetMet = false;
    if (!Results.IsValidIndex(BestResultIndex)) return;

    auto AddSource = [this, &Results](int32 Index)
    {
        if (!Results.IsValidIndex(Index) ||
            ValidationResults.ContainsByPredicate([Index, &Results](const FGraphicsValidationResult& Entry)
            {
                return Results[Entry.SourceResultIndex].AppliedConfiguration == Results[Index].AppliedConfiguration;
            }))
            return;
        ValidationResults.AddDefaulted_GetRef().SourceResultIndex = Index;
    };
    AddSource(BestResultIndex);

    int32 BorderlineIndex = INDEX_NONE;
    for (int32 Index = 1; Index < Results.Num(); ++Index)
    {
        const FGraphicsExperimentResult& Result = Results[Index];
        if (Result.bQualityRecoveryStep && Result.bTargetEvaluationValid && !Result.bMetTargetFPS &&
            Result.Performance.AverageFPS >= TargetFPS - 1.0f &&
            (BorderlineIndex == INDEX_NONE ||
                Result.Performance.AverageFPS > Results[BorderlineIndex].Performance.AverageFPS))
            BorderlineIndex = Index;
    }
    AddSource(BorderlineIndex);
    if (Results.IsValidIndex(FastestResultIndex) && Results[FastestResultIndex].bMetTargetFPS)
        AddSource(FastestResultIndex);
}

bool FOptimizationManager::NextValidationConfiguration(const TArray<FGraphicsExperimentResult>& Results,
    Scalability::FQualityLevels& OutConfiguration, FString& OutLabel)
{
    if (!ValidationResults.IsValidIndex(CurrentValidationIndex)) return false;
    const FGraphicsValidationResult& Entry = ValidationResults[CurrentValidationIndex];
    if (!Results.IsValidIndex(Entry.SourceResultIndex) ||
        Entry.RunResultIndices.Num() >= ValidationRunsPerCandidate) return false;
    const FGraphicsExperimentResult& Source = Results[Entry.SourceResultIndex];
    OutConfiguration = Source.AppliedConfiguration;
    OutLabel = FGraphicsOptimizerLocalization::Printf(TEXT("Validation %d/%d: %s"),
        Entry.RunResultIndices.Num() + 1, ValidationRunsPerCandidate, *Source.Label);
    return true;
}

void FOptimizationManager::RecordValidationResult(const TArray<FGraphicsExperimentResult>& Results, int32 ResultIndex)
{
    if (!ValidationResults.IsValidIndex(CurrentValidationIndex) || !Results.IsValidIndex(ResultIndex)) return;
    if (!Results.IsValidIndex(FastestResultIndex) ||
        Results[ResultIndex].Performance.AverageFPS > Results[FastestResultIndex].Performance.AverageFPS)
        FastestResultIndex = ResultIndex;
    FGraphicsValidationResult& Entry = ValidationResults[CurrentValidationIndex];
    Entry.RunResultIndices.Add(ResultIndex);
    if (Entry.RunResultIndices.Num() < ValidationRunsPerCandidate) return;

    TArray<double> FPSValues;
    Entry.bAllTargetEvaluationsValid = true;
    for (int32 RunIndex : Entry.RunResultIndices)
    {
        const FGraphicsExperimentResult& Run = Results[RunIndex];
        FPSValues.Add(Run.Performance.AverageFPS);
        if (!Run.bTargetEvaluationValid) Entry.bAllTargetEvaluationsValid = false;
        if (Run.bMetTargetFPS) ++Entry.TargetMetRuns;
    }
    FPSValues.Sort();
    Entry.MinimumFPS = FPSValues[0];
    Entry.MaximumFPS = FPSValues.Last();
    Entry.MedianFPS = FPSValues[FPSValues.Num() / 2];
    Entry.bStableTargetMet = Entry.bAllTargetEvaluationsValid && Entry.TargetMetRuns == ValidationRunsPerCandidate;
    ++CurrentValidationIndex;
}

void FOptimizationManager::FinalizeValidation(const TArray<FGraphicsExperimentResult>& Results)
{
    if (bValidationComplete || CurrentValidationIndex < ValidationResults.Num()) return;
    if (Results.IsEmpty()) { bValidationComplete = true; return; }
    int32 ChosenIndex = INDEX_NONE;
    const Scalability::FQualityLevels& Original = Results[0].AppliedConfiguration;
    for (int32 Index = 0; Index < ValidationResults.Num(); ++Index)
    {
        const FGraphicsValidationResult& Entry = ValidationResults[Index];
        if (!Entry.bStableTargetMet) continue;
        if (ChosenIndex == INDEX_NONE)
        {
            ChosenIndex = Index;
            continue;
        }
        const FGraphicsValidationResult& Chosen = ValidationResults[ChosenIndex];
        const int32 Changes = CountChangedSettings(Original, Results[Entry.SourceResultIndex].AppliedConfiguration);
        const int32 ChosenChanges = CountChangedSettings(Original, Results[Chosen.SourceResultIndex].AppliedConfiguration);
        if (Changes < ChosenChanges || (Changes == ChosenChanges && Entry.MedianFPS > Chosen.MedianFPS))
            ChosenIndex = Index;
    }
    if (ChosenIndex != INDEX_NONE)
    {
        BestResultIndex = ValidationResults[ChosenIndex].SourceResultIndex;
        bValidationTargetMet = true;
    }
    bValidationComplete = true;
}

const FGraphicsValidationResult* FOptimizationManager::GetSelectedValidation() const
{
    for (const FGraphicsValidationResult& Entry : ValidationResults)
    {
        if (Entry.SourceResultIndex == BestResultIndex) return &Entry;
    }
    return nullptr;
}
