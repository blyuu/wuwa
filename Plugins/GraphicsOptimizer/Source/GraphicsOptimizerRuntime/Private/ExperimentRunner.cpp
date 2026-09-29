#include "ExperimentRunner.h"
#include "GraphicsOptimizerRuntime.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "RHIGlobals.h"

namespace
{
const TCHAR* StateName(EGraphicsOptimizerState State)
{
    switch (State)
    {
    case EGraphicsOptimizerState::Idle: return TEXT("Idle");
    case EGraphicsOptimizerState::Preparing: return TEXT("Preparing");
    case EGraphicsOptimizerState::WarmupBaseline: return TEXT("WarmupBaseline");
    case EGraphicsOptimizerState::MeasuringBaseline: return TEXT("MeasuringBaseline");
    case EGraphicsOptimizerState::ApplyingConfiguration: return TEXT("ApplyingConfiguration");
    case EGraphicsOptimizerState::WarmupExperiment: return TEXT("WarmupExperiment");
    case EGraphicsOptimizerState::MeasuringExperiment: return TEXT("MeasuringExperiment");
    case EGraphicsOptimizerState::ProcessingResult: return TEXT("ProcessingResult");
    case EGraphicsOptimizerState::Restoring: return TEXT("Restoring");
    case EGraphicsOptimizerState::Completed: return TEXT("Completed");
    case EGraphicsOptimizerState::Failed: return TEXT("Failed");
    }
    return TEXT("Unknown");
}
}

bool FExperimentRunner::IsRunning() const
{
    return State != EGraphicsOptimizerState::Idle && State != EGraphicsOptimizerState::Completed && State != EGraphicsOptimizerState::Failed;
}

void FExperimentRunner::SetState(EGraphicsOptimizerState Next)
{
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][State] %s -> %s"), StateName(State), StateName(Next));
    State = Next;
}

void FExperimentRunner::BuildCandidates()
{
    Candidates.Reset();
    const Scalability::FQualityLevels& Original = Settings.GetOriginal();
    const EGraphicsSetting QualitySettings[] = {
        EGraphicsSetting::Shadow, EGraphicsSetting::GlobalIllumination, EGraphicsSetting::Reflection,
        EGraphicsSetting::PostProcess, EGraphicsSetting::Texture, EGraphicsSetting::Effects
    };
    for (EGraphicsSetting Setting : QualitySettings)
    {
        const float Current = FGraphicsSettingManager::GetValue(Original, Setting);
        if (Current >= 1.0f)
        {
            FGraphicsExperimentCandidate& Candidate = Candidates.AddDefaulted_GetRef();
            Candidate.Setting = Setting;
            Candidate.Value = Current - 1.0f;
            Candidate.Label = FString::Printf(TEXT("%s %.0f"), FGraphicsSettingManager::GetSettingName(Setting), Candidate.Value);
        }
    }
    const float BaselineResolution = Original.ResolutionQuality > 0.0f
        ? Original.ResolutionQuality : Options.ObservedBaselineResolutionScale;
    AddResolutionCandidate(BaselineResolution);
    if (BaselineResolution < 20.0f && Original.ResolutionQuality == 0.0f)
    {
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Experiment] Resolution scale uses the project default; waiting for a stable PIE primary fraction during the baseline measurement."));
    }
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Experiment] Prepared %d single-setting candidates; configured resolution=%.1f%%, observed baseline=%.1f%%, resolution candidate=%s."),
        Candidates.Num(), Original.ResolutionQuality, Options.ObservedBaselineResolutionScale,
        Candidates.ContainsByPredicate([](const FGraphicsExperimentCandidate& Candidate) { return Candidate.Setting == EGraphicsSetting::ResolutionScale; }) ? TEXT("yes") : TEXT("no"));
}

void FExperimentRunner::AddResolutionCandidate(float BaselineResolution)
{
    if (BaselineResolution < 20.0f ||
        Candidates.ContainsByPredicate([](const FGraphicsExperimentCandidate& Candidate) { return Candidate.Setting == EGraphicsSetting::ResolutionScale; }))
    {
        return;
    }

    FGraphicsExperimentCandidate& Candidate = Candidates.AddDefaulted_GetRef();
    Candidate.Setting = EGraphicsSetting::ResolutionScale;
    Candidate.Value = FMath::Max(10.0f, FMath::FloorToFloat(BaselineResolution - 10.0f));
    Candidate.Label = Settings.GetOriginal().ResolutionQuality == 0.0f
        ? FString::Printf(TEXT("Resolution Scale %.0f%% (from observed %.0f%% default)"), Candidate.Value, BaselineResolution)
        : FString::Printf(TEXT("Resolution Scale %.0f%%"), Candidate.Value);
}

bool FExperimentRunner::Start(const FGraphicsBenchmarkOptions& InOptions, UGameViewportClient* InViewport)
{
    if (IsRunning()) return false;
    if (InOptions.TargetFPS <= 0.0f || InOptions.WarmupSeconds < 0.0f || InOptions.MeasurementSeconds <= 0.0f ||
        InOptions.MinimumSamples < 1 || InOptions.MinimumUsefulGainFPS < 0.0f)
    {
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Benchmark] Invalid benchmark options."));
        return false;
    }
    Options = InOptions;
    Viewport = InViewport;
    Results.Reset();
    Optimization.Reset();
    LastError.Empty();
    CandidateIndex = 0;
    bMeasuringOptimization = false;
    bMeasuringValidation = false;
    ActiveOptimizationLabel.Empty();
    CurrentObservedPrimaryResolutionScale = 0.0f;
    SetState(EGraphicsOptimizerState::Preparing);
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Hardware] GPU: %s"), *GRHIAdapterName);
    const IConsoleVariable* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
    const IConsoleVariable* MaxFPS = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"));
    FrameRateCapFPS = GEngine ? GEngine->GetMaxTickRate(0.0f) : 0.0f;
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Benchmark] Target=%.1f FPS, warmup=%.1f sec, measure=%.1f sec, r.VSync=%d, t.MaxFPS=%.1f"),
        Options.TargetFPS, Options.WarmupSeconds, Options.MeasurementSeconds,
        VSync ? VSync->GetInt() : -1, MaxFPS ? MaxFPS->GetFloat() : -1.0f);
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Benchmark] Effective max tick rate=%.1f FPS (0 means no engine tick cap)."), FrameRateCapFPS);
    if (FrameRateCapFPS > 0.0f && Options.TargetFPS >= FrameRateCapFPS - 0.5f)
    {
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Benchmark] Target FPS is at or above the engine tick cap; target evaluation will be inconclusive near the cap."));
    }
    if (!Settings.CaptureOriginal()) { Abort(TEXT("Failed to read current graphics settings.")); return false; }
    BuildCandidates();
    AppliedConfiguration = Settings.GetOriginal();
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Benchmark] Starting baseline benchmark."));
    Benchmark.BeginWarmup(Options.WarmupSeconds);
    SetState(EGraphicsOptimizerState::WarmupBaseline);
    return true;
}

bool FExperimentRunner::CompleteMeasurement(bool bBaseline)
{
    const float ExpectedPrimaryPercentage = AppliedConfiguration.ResolutionQuality > 0.0f
        ? AppliedConfiguration.ResolutionQuality : Options.ObservedBaselineResolutionScale;
    if (!bBaseline && ExpectedPrimaryPercentage > 0.0f &&
        (bMeasuringOptimization ||
            !FMath::IsNearlyEqual(AppliedConfiguration.ResolutionQuality, Settings.GetOriginal().ResolutionQuality)))
    {
        if (CurrentObservedPrimaryResolutionScale <= 0.0f ||
            FMath::Abs(CurrentObservedPrimaryResolutionScale - ExpectedPrimaryPercentage) > 1.0f)
        {
            Abort(FString::Printf(TEXT("Expected primary resolution %.1f%% was not verified in the PIE view (observed %.1f%%)."),
                ExpectedPrimaryPercentage, CurrentObservedPrimaryResolutionScale));
            return false;
        }
    }
    SetState(EGraphicsOptimizerState::ProcessingResult);
    FGraphicsPerformanceSummary Summary = Benchmark.FinishMeasurement();
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Performance] Samples=%d, FPS=%.2f, Frame=%.2f ms, Game=%.2f ms (%d), RenderBusy=%.4f ms (%d), RenderCritical=%.4f ms (%d), GPU=%.2f ms (%d), 1%% low=%.2f"),
        Summary.FrameSamples, Summary.AverageFPS, Summary.AverageFrameMS, Summary.AverageGameMS, Summary.GameSamples,
        Summary.AverageRenderMS, Summary.RenderSamples, Summary.AverageRenderCriticalMS, Summary.RenderCriticalSamples,
        Summary.AverageGPUMS, Summary.GPUSamples, Summary.OnePercentLowFPS);
    if (Summary.FrameSamples < Options.MinimumSamples)
    {
        Abort(FString::Printf(TEXT("Insufficient frame samples: expected %d, received %d."), Options.MinimumSamples, Summary.FrameSamples));
        return false;
    }
    if (Summary.LongHitchSamples > 0)
    {
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Performance] %d frames took at least 250 ms; longest=%.1f ms. This run may not be comparable."),
            Summary.LongHitchSamples, Summary.LongestFrameMS);
    }
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][StatUnit] Samples=%d, Frame=%.2f ms, Game=%.2f ms, Draw=%.2f ms, GPU=%.2f ms"),
        Summary.StatUnitSamples, Summary.StatUnitFrameMS, Summary.StatUnitGameMS, Summary.StatUnitRenderMS, Summary.StatUnitGPUMS);
    if (Summary.StatUnitSamples == 0)
    {
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][StatUnit] No viewport stat samples; verify the Unit overlay is visible in the PIE viewport."));
    }
    if (Summary.GPUSamples == 0)
    {
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Performance] GPU timing unavailable; GPU comparison will be omitted."));
    }
    if (Summary.GameSamples == 0 || Summary.RenderSamples == 0)
    {
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Performance] Thread timing incomplete: game=%d, render=%d valid samples."), Summary.GameSamples, Summary.RenderSamples);
    }
    if (Summary.RenderSamples > 0 && Summary.AverageRenderMS < 0.01)
    {
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Performance] Render timing averages %.4f ms; treating it as unavailable for this run."), Summary.AverageRenderMS);
        Summary.RenderSamples = 0;
        Summary.AverageRenderMS = 0.0;
    }
    if (Summary.RenderCriticalSamples > 0 && Summary.AverageRenderCriticalMS < 0.01)
    {
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Performance] Render critical-path timing also averages only %.4f ms; treating it as unavailable."), Summary.AverageRenderCriticalMS);
        Summary.RenderCriticalSamples = 0;
        Summary.AverageRenderCriticalMS = 0.0;
    }
    if (Summary.StatUnitRenderMS >= 0.01)
    {
        if (Summary.RenderSamples == 0)
        {
            UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][StatUnit] Viewport Draw=%.2f ms while end-frame RenderBusy is unavailable; timing sources disagree."), Summary.StatUnitRenderMS);
        }
        else if (FMath::Abs(Summary.AverageRenderMS - Summary.StatUnitRenderMS) > FMath::Max(2.0, Summary.StatUnitRenderMS * 0.25))
        {
            UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][StatUnit] Viewport Draw and end-frame RenderBusy differ: %.2f vs %.2f ms."), Summary.StatUnitRenderMS, Summary.AverageRenderMS);
        }
    }
    FGraphicsExperimentResult& Result = Results.AddDefaulted_GetRef();
    Result.ExperimentNumber = Results.Num() - 1;
    Result.Label = bBaseline ? TEXT("Baseline") :
        bMeasuringOptimization ? ActiveOptimizationLabel : Candidates[CandidateIndex].Label;
    if (!bBaseline && !bMeasuringOptimization) Result.Candidate = Candidates[CandidateIndex];
    Result.bOptimizationStep = bMeasuringOptimization;
    Result.bQualityRecoveryStep = bMeasuringOptimization && !bMeasuringValidation && Optimization.IsRecoveringQuality();
    Result.bValidationStep = bMeasuringValidation;
    Result.AppliedConfiguration = AppliedConfiguration;
    Result.Performance = Summary;
    Result.FrameRateCapFPS = FrameRateCapFPS;
    Result.ObservedPrimaryResolutionScale = CurrentObservedPrimaryResolutionScale;
    Result.bFPSComparisonLimited = FrameRateCapFPS > 0.0f && Summary.AverageFPS >= FrameRateCapFPS * 0.98f;
    Result.bTargetEvaluationValid = !(Result.bFPSComparisonLimited && Options.TargetFPS >= FrameRateCapFPS - 0.5f);
    Result.bMetTargetFPS = Result.bTargetEvaluationValid && Summary.AverageFPS >= Options.TargetFPS;
    if (!bBaseline)
    {
        Result.FPSDifference = Summary.AverageFPS - Results[0].Performance.AverageFPS;
        Result.bGPUComparisonAvailable = Summary.GPUSamples > 0 && Results[0].Performance.GPUSamples > 0;
        if (Result.bGPUComparisonAvailable) Result.GPUTimeDifferenceMS = Summary.AverageGPUMS - Results[0].Performance.AverageGPUMS;
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Result] Experiment #%d %s: FPS delta=%+.2f (capped=%s), GPU delta=%+.2f ms (valid=%s), target=%s"),
            Result.ExperimentNumber, *Result.Label, Result.FPSDifference,
            Result.bFPSComparisonLimited ? TEXT("yes") : TEXT("no"), Result.GPUTimeDifferenceMS,
            Result.bGPUComparisonAvailable ? TEXT("yes") : TEXT("no"),
            !Result.bTargetEvaluationValid ? TEXT("inconclusive (cap)") : Result.bMetTargetFPS ? TEXT("met") : TEXT("missed"));
    }
    else
    {
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Result] Baseline completed."));
        const bool bHadResolutionCandidate = Candidates.ContainsByPredicate([](const FGraphicsExperimentCandidate& Candidate) { return Candidate.Setting == EGraphicsSetting::ResolutionScale; });
        if (!bHadResolutionCandidate && CurrentObservedPrimaryResolutionScale >= 20.0f)
        {
            const float BaselineResolution = Settings.GetOriginal().ResolutionQuality > 0.0f
                ? Settings.GetOriginal().ResolutionQuality : CurrentObservedPrimaryResolutionScale;
            Options.ObservedBaselineResolutionScale = CurrentObservedPrimaryResolutionScale;
            AddResolutionCandidate(BaselineResolution);
            if (Candidates.ContainsByPredicate([](const FGraphicsExperimentCandidate& Candidate) { return Candidate.Setting == EGraphicsSetting::ResolutionScale; }))
            {
                UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Experiment] Added resolution candidate after baseline: observed %.1f%%, requested %.1f%%; total single-setting candidates=%d."),
                    CurrentObservedPrimaryResolutionScale, Candidates.Last().Value, Candidates.Num());
            }
            else
            {
                UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Experiment] Resolution candidate remains unavailable: configured %.1f%%, observed %.1f%%."),
                    Settings.GetOriginal().ResolutionQuality, CurrentObservedPrimaryResolutionScale);
            }
        }
        else if (!bHadResolutionCandidate)
        {
            UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Experiment] No resolution candidate: configured %.1f%%, baseline observation %.1f%%. The PIE viewport did not provide a usable primary fraction."),
                Settings.GetOriginal().ResolutionQuality, CurrentObservedPrimaryResolutionScale);
        }
    }
    return true;
}

void FExperimentRunner::AdvanceExperiment()
{
    if (CandidateIndex >= Candidates.Num())
    {
        if (Options.bRunOptimization)
        {
            Optimization.Initialize(Results, Options.MinimumUsefulGainFPS);
            AdvanceOptimization();
        }
        else FinishRun(false);
        return;
    }
    SetState(EGraphicsOptimizerState::ApplyingConfiguration);
    const FGraphicsExperimentCandidate& Candidate = Candidates[CandidateIndex];
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Experiment] #%d started: %s, requested=%.1f"), CandidateIndex + 1, *Candidate.Label, Candidate.Value);
    if (!Settings.ApplyCandidate(Candidate, AppliedConfiguration)) { Abort(TEXT("Failed to apply experiment settings.")); return; }
    Benchmark.BeginWarmup(Options.WarmupSeconds);
    SetState(EGraphicsOptimizerState::WarmupExperiment);
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Benchmark] Experiment #%d warmup started."), CandidateIndex + 1);
}

void FExperimentRunner::AdvanceOptimization()
{
    Scalability::FQualityLevels Requested;
    FString Label;
    bool bHasNext = false;
    if (!bMeasuringValidation)
    {
        bHasNext = Optimization.NextConfiguration(Results, Requested, Label);
        if (!bHasNext)
        {
            const int32 FastestIndex = Optimization.GetFastestResultIndex();
            if (FrameRateCapFPS > 0.0f && Options.TargetFPS >= FrameRateCapFPS - 0.5f &&
                Results.IsValidIndex(FastestIndex) && Results[FastestIndex].bFPSComparisonLimited)
            {
                UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Validation] Skipped: effective max tick rate %.1f FPS prevents a valid %.1f FPS target verdict near the cap."),
                    FrameRateCapFPS, Options.TargetFPS);
                FinishRun(false);
                return;
            }
            Optimization.BeginValidation(Results, Options.TargetFPS);
            bMeasuringValidation = true;
            UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Validation] Starting %d fresh runs for each of %d configurations."),
                FOptimizationManager::ValidationRunsPerCandidate, Optimization.GetValidationResults().Num());
        }
    }
    if (bMeasuringValidation && !bHasNext)
        bHasNext = Optimization.NextValidationConfiguration(Results, Requested, Label);
    if (!bHasNext)
    {
        Optimization.FinalizeValidation(Results);
        FinishRun(false);
        return;
    }
    SetState(EGraphicsOptimizerState::ApplyingConfiguration);
    ActiveOptimizationLabel = MoveTemp(Label);
    bMeasuringOptimization = true;
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Optimization] Testing %s"), *ActiveOptimizationLabel);
    if (!Settings.ApplyConfiguration(Requested, AppliedConfiguration))
    {
        Abort(TEXT("Failed to apply combined graphics settings."));
        return;
    }
    Benchmark.BeginWarmup(Options.WarmupSeconds);
    SetState(EGraphicsOptimizerState::WarmupExperiment);
}

void FExperimentRunner::Tick(float ObservedPrimaryResolutionScale)
{
    CurrentObservedPrimaryResolutionScale = ObservedPrimaryResolutionScale;
    switch (State)
    {
    case EGraphicsOptimizerState::WarmupBaseline:
    case EGraphicsOptimizerState::WarmupExperiment:
        if (Benchmark.IsPeriodComplete())
        {
            const bool bBaseline = State == EGraphicsOptimizerState::WarmupBaseline;
            Benchmark.BeginMeasurement(Options.MeasurementSeconds, Viewport.Get());
            SetState(bBaseline ? EGraphicsOptimizerState::MeasuringBaseline : EGraphicsOptimizerState::MeasuringExperiment);
            UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Benchmark] %s measurement started."), bBaseline ? TEXT("Baseline") : TEXT("Experiment"));
        }
        break;
    case EGraphicsOptimizerState::MeasuringBaseline:
    case EGraphicsOptimizerState::MeasuringExperiment:
    {
        const bool bBaseline = State == EGraphicsOptimizerState::MeasuringBaseline;
        Benchmark.TickMeasurement();
        if (!Benchmark.IsPeriodComplete()) break;
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Benchmark] %s measurement complete."), bBaseline ? TEXT("Baseline") : TEXT("Experiment"));
        if (!CompleteMeasurement(bBaseline)) break;
        if (bBaseline) AdvanceExperiment();
        else
        {
            SetState(EGraphicsOptimizerState::Restoring);
            if (!Settings.RestoreOriginal()) { Abort(TEXT("Failed to restore original graphics settings.")); break; }
            if (bMeasuringOptimization)
            {
                if (bMeasuringValidation)
                {
                    Optimization.RecordValidationResult(Results, Results.Num() - 1);
                    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Validation] Run completed: %s."), *ActiveOptimizationLabel);
                }
                else
                {
                    const bool bRecoveryTest = Optimization.IsRecoveringQuality();
                    Optimization.RecordResult(Results, Results.Num() - 1);
                    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Optimization] %s test completed; best result index=%d."),
                        bRecoveryTest ? TEXT("Quality recovery") : TEXT("Combined"), Optimization.GetBestResultIndex());
                }
                AdvanceOptimization();
            }
            else
            {
                UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Experiment] #%d completed."), CandidateIndex + 1);
                ++CandidateIndex;
                AdvanceExperiment();
            }
        }
        break;
    }
    default: break;
    }
}

void FExperimentRunner::FinishRun(bool bFailed)
{
    SetState(EGraphicsOptimizerState::Restoring);
    const bool bRestored = Settings.RestoreOriginal();
    if (!bRestored) LastError = TEXT("Original settings could not be restored.");
    SetState(bFailed || !bRestored ? EGraphicsOptimizerState::Failed : EGraphicsOptimizerState::Completed);
    if (bFailed || !bRestored)
    {
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Benchmark] Run failed. Results=%d. %s"), Results.Num(), *LastError);
    }
    else
    {
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Benchmark] All experiments completed. Results=%d."), Results.Num());
        if (Options.bRunOptimization && Results.IsValidIndex(Optimization.GetBestResultIndex()))
        {
            const FGraphicsExperimentResult& Best = Results[Optimization.GetBestResultIndex()];
            const FGraphicsExperimentResult& Fastest = Results.IsValidIndex(Optimization.GetFastestResultIndex())
                ? Results[Optimization.GetFastestResultIndex()] : Best;
            const FGraphicsValidationResult* SelectedValidation = Optimization.GetSelectedValidation();
            const FString ValidationSummary = SelectedValidation
                ? FString::Printf(TEXT(", validation median %.4f FPS, range %.4f-%.4f FPS, %d/%d target met"),
                    SelectedValidation->MedianFPS, SelectedValidation->MinimumFPS, SelectedValidation->MaximumFPS,
                    SelectedValidation->TargetMetRuns, FOptimizationManager::ValidationRunsPerCandidate)
                : FString();
            UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Optimization] %s; quality-preserving candidate: %s, source run %.2f FPS%s. Original settings restored."),
                Optimization.HasMetTarget() ? TEXT("Target met") :
                    Optimization.IsValidationComplete() && Optimization.HadTargetBeforeValidation() ? TEXT("Target not repeatably met") :
                    !Fastest.bTargetEvaluationValid ? TEXT("Target inconclusive (frame cap)") :
                    TEXT("Target missed"),
                *Best.Label, Best.Performance.AverageFPS, *ValidationSummary);
            for (const FGraphicsValidationResult& Entry : Optimization.GetValidationResults())
            {
                UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Validation] %s: median %.4f FPS, range %.4f-%.4f FPS, target met %d/%d, stable=%s."),
                    Results.IsValidIndex(Entry.SourceResultIndex) ? *Results[Entry.SourceResultIndex].Label : TEXT("Unknown"),
                    Entry.MedianFPS, Entry.MinimumFPS, Entry.MaximumFPS, Entry.TargetMetRuns,
                    FOptimizationManager::ValidationRunsPerCandidate, Entry.bStableTargetMet ? TEXT("yes") : TEXT("no"));
            }
            if (Results.IsValidIndex(Optimization.GetFastestResultIndex()) &&
                Fastest.AppliedConfiguration != Best.AppliedConfiguration)
            {
                UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Optimization] Fastest observation: %s, %.2f FPS; selected gain threshold=%.2f FPS per extra setting."),
                    *Fastest.Label, Fastest.Performance.AverageFPS, Options.MinimumUsefulGainFPS);
            }
        }
    }
}

void FExperimentRunner::Abort(const FString& Reason)
{
    LastError = Reason;
    UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][ERROR] %s State=%s, experiment=%d"), *Reason, StateName(State), CandidateIndex + 1);
    FinishRun(true);
}

void FExperimentRunner::Cancel()
{
    if (IsRunning()) Abort(TEXT("Benchmark cancelled by user."));
}
