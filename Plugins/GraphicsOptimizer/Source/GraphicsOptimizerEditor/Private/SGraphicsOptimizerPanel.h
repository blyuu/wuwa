#pragma once

#include "Widgets/SCompoundWidget.h"

class UGraphicsOptimizerSubsystem;
struct FSlateDynamicImageBrush;
struct FSlateBrush;

class SGraphicsOptimizerPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGraphicsOptimizerPanel) {}
    SLATE_END_ARGS()
    void Construct(const FArguments& InArgs);
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    TSharedRef<SWidget> BuildContent();
    FReply ToggleLanguage();
    UGraphicsOptimizerSubsystem* GetSubsystem() const;
    FText GetStatusText() const;
    FText GetResultsText() const;
    FText GetStatUnitText() const;
    FText GetOptimizationText() const;
    FText GetSelectedResultText() const;
    FText GetMotionText() const;
    FText GetShadowCandidateText() const;
    FText GetShadowStudyText() const;
    FText GetGpuTraceText() const;
    FText GetFullAnalysisText() const;
    FText GetEngineSceneText() const;
    FText GetLumenDetailText() const;
    const FSlateBrush* GetComparisonBrush() const;
    const FSlateBrush* GetShadowBrush() const;
    int32 GetSelectedResultIndex() const;
    FReply RunBenchmark();
    FReply RunOptimization();
    FReply RunSettingsBenchmark();
    FReply StartRun(bool bOptimize, bool bFullAnalysis = false);
    FReply CancelBenchmark();
    FReply CopyReport();
    FReply CopySettingsReport();
    FReply CopyEngineSceneReport();
    FReply CopyLumenDetailReport();
    FReply CopyAccumulatedReports();
    FReply OpenReportHistory();
    void SaveGpuAnalysisHistory();
    FReply ApplySelectedResult();
    FReply SaveSelectedPreset();
    FReply ApplySavedPreset();
    FReply RestoreAppliedSettings();
    FReply CaptureVisualComparison();
    FReply OpenComparisonImage();
    FReply RunMotionComparison();
    FReply OpenMotionVideo();
    FReply ScanShadowCasters();
    FReply RunShadowStudy();
    FReply OpenShadowStudyImage();
    FReply CopyShadowStudyReport();
    FReply CaptureGpuTrace();
    FReply AnalyzeGpuTrace();
    FReply OpenGpuTrace();
    FReply CopyGpuTraceReport();
    FReply RunGpuPassStudy();
    FReply OpenGpuPassStudyGIImage();
    FReply OpenGpuPassStudyShadowImage();
    FReply OpenGpuPassStudyTrace(int32 Arm);
    FReply ApplyFullRecommendation();
    FReply KeepOriginalFullAnalysis();
    FReply CopyFullAnalysisReport();
    FReply ScanEngineScene();
    FReply RunLumenDetailStudy();
    FReply OpenLumenDetailImage();
    float TargetFPS = 60.0f;
    float MeasurementSeconds = 5.0f;
    float MinimumUsefulGainFPS = 0.75f;
    bool bFreezeGameplay = false;
    int32 SelectedResultIndex = -1; // -1 selects the optimizer's recommendation.
    int32 ShadowCandidateIndex = 0;
    mutable FString CachedComparisonImagePath;
    mutable TSharedPtr<FSlateDynamicImageBrush> ComparisonBrush;
    mutable FString CachedShadowImagePath;
    mutable TSharedPtr<FSlateDynamicImageBrush> ShadowBrush;
    FString GpuPassAnalysis;
    FString AnalyzedGpuTracePath;
    FString LastAutoAnalysisPath;
    bool bCopyFullReportOnFinish = false;
    mutable FString CachedFullAnalysisDirectory;
    mutable FString CachedFullGpuAnalysis;
};
