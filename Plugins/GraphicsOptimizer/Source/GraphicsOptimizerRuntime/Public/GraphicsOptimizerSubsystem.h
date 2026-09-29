#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "ExperimentRunner.h"
#include "GraphicsOptimizerSubsystem.generated.h"

class UGameViewportClient;
class FGraphicsResolutionProbe;
class APlayerController;
class APawn;
class UStaticMeshComponent;
class IConsoleVariable;

struct FShadowCasterGroup
{
    FString MeshPath;
    TArray<TWeakObjectPtr<UStaticMeshComponent>> Components;
    int32 StaticCount = 0;
    int32 StationaryCount = 0;
    int32 MovableCount = 0;
};

struct FShadowOriginalFlag
{
    TWeakObjectPtr<UStaticMeshComponent> Component;
    bool bCastShadow = false;
};

struct FFullShadowObservation
{
    FString MeshPath;
    FString ImagePath;
    FString Report;
    double FPSGain = 0.0;
    double GPUMSGain = 0.0;
};

UCLASS()
class GRAPHICSOPTIMIZERRUNTIME_API UGraphicsOptimizerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    enum class EComparisonStage : uint8 { None, Baseline, Selected };
    enum class EMotionStage : uint8 { None, BaselinePerformance, SelectedPerformance, BaselineVideo, SelectedVideo };
    enum class EShadowStudyStage : uint8 { None, WarmupBaseline, MeasuringBaseline, WarmupCandidate, MeasuringCandidate };
    enum class EGpuTraceStage : uint8 { None, Warmup, Measuring };
    enum class ELumenStudyStage : uint8 { None, BaselineWarmup, BaselineMeasure, CandidateWarmup, CandidateMeasure };
    enum class EFullAnalysisStage : uint8 { None, Benchmark, GpuPasses, Shadow, Lumen, Visual, Complete, Failed };

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool StartBenchmark(const FGraphicsBenchmarkOptions& Options);
    bool StartFullAnalysis(const FGraphicsBenchmarkOptions& Options);
    bool IsFullAnalysisRunning() const { return FullAnalysisStage == EFullAnalysisStage::Benchmark ||
        FullAnalysisStage == EFullAnalysisStage::GpuPasses || FullAnalysisStage == EFullAnalysisStage::Shadow ||
        FullAnalysisStage == EFullAnalysisStage::Lumen ||
        FullAnalysisStage == EFullAnalysisStage::Visual; }
    bool IsFullAnalysisComplete() const { return FullAnalysisStage == EFullAnalysisStage::Complete; }
    FString GetFullAnalysisProgress() const;
    const FString& GetFullAnalysisReport() const { return FullAnalysisReport; }
    const FString& GetFullAnalysisDirectory() const { return FullAnalysisDirectory; }
    bool ApplyFullAnalysisRecommendation();
    bool KeepOriginalAfterFullAnalysis();
    void CancelBenchmark();
    bool ApplyResult(int32 ResultIndex);
    bool SaveResultPreset(int32 ResultIndex);
    bool StartVisualComparison(int32 ResultIndex);
    void CancelVisualComparison();
    bool IsVisualComparisonRunning() const { return ComparisonStage != EComparisonStage::None; }
    const FString& GetComparisonImagePath() const { return ComparisonImagePath; }
    const FString& GetComparisonDirectory() const { return ComparisonDirectory; }
    FIntPoint GetComparisonSize() const { return ComparisonSize; }
    bool StartMotionComparison(int32 ResultIndex);
    void CancelMotionComparison();
    bool IsMotionComparisonRunning() const { return MotionStage != EMotionStage::None; }
    const FString& GetMotionVideoPath() const { return MotionVideoPath; }
    const FString& GetMotionReport() const { return MotionReport; }
    bool ScanShadowCasters();
    bool StartShadowStudy(int32 CandidateIndex, float MeasurementSeconds);
    void CancelShadowStudy();
    bool IsShadowStudyRunning() const { return ShadowStudyStage != EShadowStudyStage::None; }
    int32 GetShadowCandidateCount() const { return ShadowCandidates.Num(); }
    const FString& GetShadowCandidateList() const { return ShadowCandidateList; }
    const FString& GetShadowStudyReport() const { return ShadowStudyReport; }
    const FString& GetShadowStudyImagePath() const { return ShadowStudyImagePath; }
    FIntPoint GetShadowStudySize() const { return ShadowImageSize; }
    bool StartGpuTraceCapture(float MeasurementSeconds, bool bFreezeScene);
    bool StartGpuPassStudy(float MeasurementSeconds);
    void CancelGpuTraceCapture();
    bool IsGpuTraceCaptureRunning() const { return GpuTraceStage != EGpuTraceStage::None; }
    bool HasGpuPassStudy() const { return bGpuPassStudyCompleted && GpuTraceStage == EGpuTraceStage::None &&
        GpuPassStudyTraces.Num() == 3 && !GpuPassStudyTraces[0].IsEmpty() &&
        !GpuPassStudyTraces[1].IsEmpty() && !GpuPassStudyTraces[2].IsEmpty(); }
    const TArray<FString>& GetGpuPassStudyTraces() const { return GpuPassStudyTraces; }
    const TArray<FGraphicsPerformanceSummary>& GetGpuPassStudyPerformance() const { return GpuPassStudyPerformance; }
    const FString& GetGpuPassStudyGIImagePath() const { return GpuPassStudyGIImagePath; }
    const FString& GetGpuPassStudyShadowImagePath() const { return GpuPassStudyShadowImagePath; }
    const FString& GetGpuTracePath() const { return GpuTracePath; }
    const FString& GetGpuTraceReport() const { return GpuTraceReport; }
    const FString& GetGpuTraceDirectory() const { return GpuTraceDirectory; }
    int32 GetGpuTraceFrameSamples() const { return GpuTracePerformance.FrameSamples; }
    bool ScanEngineScene();
    const FString& GetEngineSceneReport() const { return EngineSceneReport; }
    bool StartLumenDetailStudy(float MeasurementSeconds);
    void CancelLumenDetailStudy();
    bool IsLumenDetailStudyRunning() const { return LumenStudyStage != ELumenStudyStage::None; }
    const FString& GetLumenDetailReport() const { return LumenStudyReport; }
    const FString& GetLumenDetailImagePath() const { return LumenStudyImagePath; }
    UFUNCTION(BlueprintCallable, Category="Graphics Optimizer")
    bool SaveCurrentSettingsAsPreset();
    UFUNCTION(BlueprintCallable, Category="Graphics Optimizer")
    bool ApplySavedPreset();
    UFUNCTION(BlueprintCallable, Category="Graphics Optimizer")
    bool RestoreAppliedConfiguration();
    UFUNCTION(BlueprintPure, Category="Graphics Optimizer")
    FString GetSavedPresetPath() const;
    const FString& GetLastActionMessage() const { return LastActionMessage; }
    bool HasAppliedConfiguration() const { return bHasAppliedConfiguration; }
    const FExperimentRunner& GetRunner() const { return Runner; }
    void ArchiveReport(const FString& RunKey, const FString& Title, const FString& Report);
    FString GetAccumulatedReports() const;
    FString GetReportHistoryDirectory() const;
    const FString& GetLatestBenchmarkReport() const { return LatestBenchmarkReport; }

private:
    void OnEndFrame();
    void RestoreGameplayPause();
    void RestoreStatUnit();
    void EnsureResolutionProbe();
    bool ApplyConfiguration(const Scalability::FQualityLevels& Configuration, const FString& Label);
    void TickVisualComparison();
    void FinishVisualComparison(bool bSuccess, const FString& Message);
    bool SaveComparisonImage(const FString& Path, const TArray<FColor>& Pixels, FIntPoint Size, bool bLog = true) const;
    void BeginMotionStage(EMotionStage NextStage);
    void TickMotionComparison();
    void SetMotionInput(int32 Direction);
    void FinishMotionComparison(bool bSuccess, const FString& Message);
    bool SaveMotionFramesThrough(int32 LastFrameIndex);
    void TickShadowStudy();
    void FinishShadowStudy(bool bSuccess, const FString& Reason);
    bool CaptureShadowStudyFrame(bool bBaseline);
    void TickGpuTraceCapture();
    void FinishGpuTraceCapture(bool bSuccess, const FString& Reason);
    void FinishGpuPassStudySegment();
    bool CaptureGpuPassStudyImage();
    void TickLumenDetailStudy();
    void FinishLumenDetailStudy(bool bSuccess, const FString& Reason);
    bool CaptureLumenDetailImage(bool bBaseline);
    void AdvanceFullAnalysis();
    void FinishFullAnalysis(bool bComplete, const FString& Reason);
    void WriteFullAnalysisReport();
    void ArchiveBenchmarkIfFinished();
    FString ReportHistorySessionDirectory;
    TMap<FString, FString> ReportHistoryPaths;
    FString BenchmarkReportId;
    FString LatestBenchmarkReport;
    FString BenchmarkReportContext;
    bool bBenchmarkReportPending = false;
    FDelegateHandle EndFrameHandle;
    FExperimentRunner Runner;
    TWeakObjectPtr<UWorld> PausedWorld;
    bool bRestoreGameplayPause = false;
    bool bPreviousPauseState = false;
    FVector FixedCameraLocation = FVector::ZeroVector;
    FRotator FixedCameraRotation = FRotator::ZeroRotator;
    bool bHasFixedCamera = false;
    TWeakObjectPtr<UGameViewportClient> StatViewport;
    bool bRestoreStatUnit = false;
    TSharedPtr<FGraphicsResolutionProbe, ESPMode::ThreadSafe> ResolutionProbe;
    Scalability::FQualityLevels OriginalBeforeApply;
    bool bHasAppliedConfiguration = false;
    FString LastActionMessage;
    EComparisonStage ComparisonStage = EComparisonStage::None;
    Scalability::FQualityLevels BeforeComparison;
    Scalability::FQualityLevels ComparisonBaseline;
    Scalability::FQualityLevels ComparisonSelected;
    float ComparisonBaselinePrimary = 0.0f;
    float ComparisonSelectedPrimary = 0.0f;
    TWeakObjectPtr<UWorld> ComparisonWorld;
    TWeakObjectPtr<UGameViewportClient> ComparisonViewport;
    bool bComparisonPausedWorld = false;
    bool bComparisonStatUnitWasEnabled = false;
    FVector ComparisonCameraLocation = FVector::ZeroVector;
    FRotator ComparisonCameraRotation = FRotator::ZeroRotator;
    FIntPoint ComparisonSize = FIntPoint::ZeroValue;
    double ComparisonStageStartSeconds = 0.0;
    int32 ComparisonStageFrames = 0;
    int32 ComparisonResultIndex = INDEX_NONE;
    TArray<FColor> ComparisonBaselinePixels;
    FString ComparisonBaselinePixelDiagnostics;
    FString ComparisonSelectedPixelDiagnostics;
    FString ComparisonDirectory;
    FString ComparisonImagePath;
    EMotionStage MotionStage = EMotionStage::None;
    bool bMotionWarmup = false;
    TWeakObjectPtr<UWorld> MotionWorld;
    TWeakObjectPtr<UGameViewportClient> MotionViewport;
    TWeakObjectPtr<APlayerController> MotionController;
    TWeakObjectPtr<APawn> MotionPawn;
    FTransform MotionStartTransform;
    FRotator MotionStartControlRotation = FRotator::ZeroRotator;
    Scalability::FQualityLevels BeforeMotion;
    Scalability::FQualityLevels MotionBaseline;
    Scalability::FQualityLevels MotionSelected;
    FBenchmarkManager MotionBenchmark;
    FGraphicsPerformanceSummary MotionBaselinePerformance;
    FGraphicsPerformanceSummary MotionSelectedPerformance;
    TArray<FVector> MotionBaselinePositions;
    TArray<FVector> MotionSelectedPositions;
    FIntPoint MotionSize = FIntPoint::ZeroValue;
    double MotionStageStartSeconds = 0.0;
    double MotionRunStartSeconds = 0.0;
    int32 MotionHeldDirection = 0;
    bool bMotionStatUnitWasEnabled = false;
    int32 MotionFrameIndex = 0;
    int32 MotionResultIndex = INDEX_NONE;
    FString MotionDirectory;
    FString MotionVideoPath;
    FString MotionReport;
    EShadowStudyStage ShadowStudyStage = EShadowStudyStage::None;
    TArray<FShadowCasterGroup> ShadowCandidates;
    TArray<FShadowOriginalFlag> ShadowOriginalFlags;
    FBenchmarkManager ShadowBenchmark;
    FGraphicsPerformanceSummary ShadowBaselinePerformance;
    FGraphicsPerformanceSummary ShadowCandidatePerformance;
    TWeakObjectPtr<UWorld> ShadowWorld;
    TWeakObjectPtr<UGameViewportClient> ShadowViewport;
    Scalability::FQualityLevels ShadowOriginalQuality;
    FVector ShadowCameraLocation = FVector::ZeroVector;
    FRotator ShadowCameraRotation = FRotator::ZeroRotator;
    FIntPoint ShadowImageSize = FIntPoint::ZeroValue;
    TArray<FColor> ShadowBaselinePixels;
    int32 ShadowSelectedCandidate = INDEX_NONE;
    float ShadowMeasurementSeconds = 5.0f;
    float ShadowFrameRateCapFPS = 0.0f;
    bool bShadowPausedWorld = false;
    FString ShadowCandidateList;
    FString ShadowStudyReport;
    FString ShadowStudyDirectory;
    FString ShadowStudyImagePath;
    bool bLastShadowStudyCompleted = false;
    EGpuTraceStage GpuTraceStage = EGpuTraceStage::None;
    FBenchmarkManager GpuTraceBenchmark;
    FGraphicsPerformanceSummary GpuTracePerformance;
    TWeakObjectPtr<UWorld> GpuTraceWorld;
    TWeakObjectPtr<UGameViewportClient> GpuTraceViewport;
    FVector GpuTraceCameraLocation = FVector::ZeroVector;
    FRotator GpuTraceCameraRotation = FRotator::ZeroRotator;
    bool bGpuTraceFreezeScene = false;
    bool bGpuTracePausedWorld = false;
    bool bGpuTraceOwnsTrace = false;
    float GpuTraceMeasurementSeconds = 5.0f;
    float GpuTraceTickCapFPS = 0.0f;
    Scalability::FQualityLevels GpuTraceQuality;
    FString GpuTraceDirectory;
    FString GpuTracePath;
    FString GpuTraceReport;
    FString GpuTraceNewChannels;
    bool bGpuPassStudy = false;
    bool bGpuPassStudyCompleted = false;
    int32 GpuPassStudySegment = 0;
    Scalability::FQualityLevels GpuPassStudyOriginalQuality;
    Scalability::FQualityLevels GpuPassStudyExpectedQuality;
    FIntPoint GpuPassStudyImageSize = FIntPoint::ZeroValue;
    TArray<FColor> GpuPassStudyBaselinePixels;
    TArray<FString> GpuPassStudyTraces;
    TArray<FGraphicsPerformanceSummary> GpuPassStudyPerformance;
    FString GpuPassStudyGIImagePath;
    FString GpuPassStudyShadowImagePath;
    FString EngineSceneReport;
    ELumenStudyStage LumenStudyStage = ELumenStudyStage::None;
    FBenchmarkManager LumenStudyBenchmark;
    FGraphicsPerformanceSummary LumenBaselinePerformance;
    FGraphicsPerformanceSummary LumenCandidatePerformance;
    TWeakObjectPtr<UWorld> LumenStudyWorld;
    TWeakObjectPtr<UGameViewportClient> LumenStudyViewport;
    IConsoleVariable* LumenStudyCVar = nullptr;
    Scalability::FQualityLevels LumenStudyQuality;
    FVector LumenStudyCameraLocation = FVector::ZeroVector;
    FRotator LumenStudyCameraRotation = FRotator::ZeroRotator;
    FIntPoint LumenStudySize = FIntPoint::ZeroValue;
    TArray<FColor> LumenBaselinePixels;
    FString LumenStudyDirectory;
    FString LumenStudyReport;
    FString LumenStudyImagePath;
    int32 LumenStudyOriginalFactor = 0;
    int32 LumenStudyCandidateFactor = 0;
    float LumenStudyMeasurementSeconds = 5.0f;
    bool bLumenStudyPausedWorld = false;
    bool bLumenStudyCompleted = false;
    EFullAnalysisStage FullAnalysisStage = EFullAnalysisStage::None;
    float FullAnalysisMeasurementSeconds = 5.0f;
    int32 FullAnalysisShadowIndex = 0;
    int32 FullAnalysisShadowLimit = 0;
    TArray<FFullShadowObservation> FullAnalysisShadowObservations;
    FString FullAnalysisDirectory;
    FString FullAnalysisReport;
    FString FullAnalysisDecision;
    FString FullAnalysisOutcome;
    TWeakObjectPtr<UWorld> FullAnalysisWorld;
    bool bFullAnalysisPausedWorld = false;
    Scalability::FQualityLevels FullAnalysisStartingQuality;
    Scalability::FQualityLevels FullAnalysisPreviousApplyOriginal;
    bool bFullAnalysisHadAppliedConfiguration = false;
};
