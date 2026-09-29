#include "SGraphicsOptimizerPanel.h"
#include "GraphicsOptimizerSubsystem.h"
#include "GraphicsSettingManager.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "RHIGlobals.h"
#include "UnrealClient.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"
#include "TraceServices/ITraceServicesModule.h"
#include "TraceServices/AnalysisService.h"
#include "TraceServices/Model/AnalysisSession.h"
#include "TraceServices/Model/TimingProfiler.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
const TCHAR* StateLabel(EGraphicsOptimizerState State)
{
    switch (State)
    {
    case EGraphicsOptimizerState::Idle: return TEXT("Idle");
    case EGraphicsOptimizerState::Preparing: return TEXT("Preparing");
    case EGraphicsOptimizerState::WarmupBaseline: return TEXT("Warming up baseline");
    case EGraphicsOptimizerState::MeasuringBaseline: return TEXT("Measuring baseline");
    case EGraphicsOptimizerState::ApplyingConfiguration: return TEXT("Applying setting");
    case EGraphicsOptimizerState::WarmupExperiment: return TEXT("Warming up experiment");
    case EGraphicsOptimizerState::MeasuringExperiment: return TEXT("Measuring experiment");
    case EGraphicsOptimizerState::ProcessingResult: return TEXT("Processing result");
    case EGraphicsOptimizerState::Restoring: return TEXT("Restoring settings");
    case EGraphicsOptimizerState::Completed: return TEXT("Completed");
    case EGraphicsOptimizerState::Failed: return TEXT("Failed");
    }
    return TEXT("Unknown");
}
}

UGraphicsOptimizerSubsystem* SGraphicsOptimizerPanel::GetSubsystem() const
{
    UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
    UGameInstance* GameInstance = PlayWorld ? PlayWorld->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UGraphicsOptimizerSubsystem>() : nullptr;
}

void SGraphicsOptimizerPanel::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot().Padding(10)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Graphics Optimizer — PIE benchmark"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Choose a study below, or expand Complete analysis to run every study together. Changes are applied only when you choose an Apply button."))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Target FPS"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SSpinBox<float>).MinValue(1.0f).MaxValue(500.0f).Value(TargetFPS)
                .OnValueChanged_Lambda([this](float Value) { TargetFPS = Value; }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Measurement duration per configuration (seconds)"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SSpinBox<float>).MinValue(1.0f).MaxValue(120.0f).Value(MeasurementSeconds)
                .OnValueChanged_Lambda([this](float Value) { MeasurementSeconds = Value; }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Minimum useful FPS gain per extra quality reduction"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SSpinBox<float>).MinValue(0.0f).MaxValue(10.0f).Value(MinimumUsefulGainFPS)
                .OnValueChanged_Lambda([this](float Value) { MinimumUsefulGainFPS = Value; }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SCheckBox)
                .IsChecked(ECheckBoxState::Unchecked)
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bFreezeGameplay = State == ECheckBoxState::Checked; })
                [ SNew(STextBlock).Text(FText::FromString(TEXT("Freeze gameplay for a static-scene comparison (combat stops)"))) ] ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetStatusText) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetStatUnitText) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Cancel and Restore")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && (S->GetRunner().IsRunning() || S->IsVisualComparisonRunning() || S->IsMotionComparisonRunning() || S->IsShadowStudyRunning() || S->IsGpuTraceCaptureRunning() || S->IsLumenDetailStudyRunning()); })
                .OnClicked(this, &SGraphicsOptimizerPanel::CancelBenchmark) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Copy Latest Results (may include separate runs)")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && !S->IsLumenDetailStudyRunning() && (!S->GetRunner().GetResults().IsEmpty() || !S->GetShadowStudyReport().IsEmpty() || !S->GetGpuTraceReport().IsEmpty() || !S->GetEngineSceneReport().IsEmpty() || !S->GetLumenDetailReport().IsEmpty()); })
                .OnClicked(this, &SGraphicsOptimizerPanel::CopyReport) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Copy Accumulated Project Reports")))
                .IsEnabled_Lambda([this]() { return GetSubsystem() != nullptr; })
                .OnClicked(this, &SGraphicsOptimizerPanel::CopyAccumulatedReports) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open Report History Folder")))
                .IsEnabled_Lambda([this]() { return GetSubsystem() != nullptr; })
                .OnClicked(this, &SGraphicsOptimizerPanel::OpenReportHistory) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SNew(SExpandableArea)
                .AreaTitle(FText::FromString(TEXT("Quality settings - measured search and selection")))
                .InitiallyCollapsed(false)
                .BodyContent()
                [ SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Compare Scalability candidates, inspect images or motion, then apply or save a selected setting."))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Run Optimization (settings only)")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetRunner().IsRunning() && !S->IsVisualComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && !S->IsLumenDetailStudyRunning(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::RunOptimization) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Run Settings Benchmark (individual changes only)")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsFullAnalysisRunning() && !S->GetRunner().IsRunning() && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && !S->IsLumenDetailStudyRunning(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::RunSettingsBenchmark) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Copy Settings Report")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetLatestBenchmarkReport().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::CopySettingsReport) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Result number to apply/save (-1 = recommended result)"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SSpinBox<int32>).MinValue(-1).MaxValue(1000).Value_Lambda([this]() { return SelectedResultIndex; })
                .OnValueChanged_Lambda([this](int32 Value) { SelectedResultIndex = Value; }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetSelectedResultText) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Capture A/B: Baseline vs Selected")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && S->GetRunner().GetState() == EGraphicsOptimizerState::Completed && S->GetRunner().GetResults().IsValidIndex(GetSelectedResultIndex()); })
                .OnClicked(this, &SGraphicsOptimizerPanel::CaptureVisualComparison) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open A/B Image at Full Size")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetComparisonImagePath().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::OpenComparisonImage) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("A/B preview: baseline on left, selected result on right"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SImage).Image(this, &SGraphicsOptimizerPanel::GetComparisonBrush) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Run 4s A/D Motion Comparison")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsMotionComparisonRunning() && !S->IsVisualComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && S->GetRunner().GetState() == EGraphicsOptimizerState::Completed && S->GetRunner().GetResults().IsValidIndex(GetSelectedResultIndex()); })
                .OnClicked(this, &SGraphicsOptimizerPanel::RunMotionComparison) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open 4s Motion A/B Video")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetMotionVideoPath().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::OpenMotionVideo) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetMotionText) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Apply Selected to PIE")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && S->GetRunner().GetState() == EGraphicsOptimizerState::Completed && S->GetRunner().GetResults().IsValidIndex(GetSelectedResultIndex()); })
                .OnClicked(this, &SGraphicsOptimizerPanel::ApplySelectedResult) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Save Selected Preset")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsGpuTraceCaptureRunning() && S->GetRunner().GetState() == EGraphicsOptimizerState::Completed && S->GetRunner().GetResults().IsValidIndex(GetSelectedResultIndex()); })
                .OnClicked(this, &SGraphicsOptimizerPanel::SaveSelectedPreset) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Apply Saved Preset to PIE")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetRunner().IsRunning() && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::ApplySavedPreset) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Restore Pre-Apply Settings")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetRunner().IsRunning() && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && S->HasAppliedConfiguration(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::RestoreAppliedSettings) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Experiment results"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetOptimizationText) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetResultsText) ]
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SNew(SExpandableArea)
                .AreaTitle(FText::FromString(TEXT("Shadow casters and GPU traces")))
                .InitiallyCollapsed(true)
                .BodyContent()
                [ SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Measure shadow-caster changes and capture GPU pass traces. Results are diagnostic; no shadow change is applied automatically."))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Local shadow-caster study (static PIE scene)"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Scan recently rendered static-mesh components that cast dynamic shadows. Static, Stationary, and Movable are listed; order is count, not GPU cost."))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Scan Shadow Casters")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetRunner().IsRunning() && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::ScanShadowCasters) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetShadowCandidateText) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Shadow mesh group number to test"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SSpinBox<int32>).MinValue(0).MaxValue(19).Value_Lambda([this]() { return ShadowCandidateIndex; })
                .OnValueChanged_Lambda([this](int32 Value) { ShadowCandidateIndex = Value; }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Benchmark Shadow Group A/B")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetRunner().IsRunning() && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && ShadowCandidateIndex < S->GetShadowCandidateCount(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::RunShadowStudy) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open Shadow A/B Image")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetShadowStudyImagePath().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::OpenShadowStudyImage) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SImage).Image(this, &SGraphicsOptimizerPanel::GetShadowBrush) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Copy Shadow Report")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetShadowStudyReport().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::CopyShadowStudyReport) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetShadowStudyText) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("GPU pass trace (current PIE settings)"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Capture a GPU trace and FPS sample after a 1s warmup. Duration and static-scene checkbox above also apply. Current graphics settings are preserved."))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Capture GPU Trace")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetRunner().IsRunning() && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::CaptureGpuTrace) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Analyze GPU Passes")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsGpuTraceCaptureRunning() && IFileManager::Get().FileSize(*S->GetGpuTracePath()) > 0; })
                .OnClicked(this, &SGraphicsOptimizerPanel::AnalyzeGpuTrace) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open Latest GPU Trace in Unreal Insights (Shadow for A/B)")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsGpuTraceCaptureRunning() && IFileManager::Get().FileSize(*S->GetGpuTracePath()) > 0; })
                .OnClicked(this, &SGraphicsOptimizerPanel::OpenGpuTrace) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Copy GPU Report and Pass Analysis")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsGpuTraceCaptureRunning() && !S->GetGpuTraceReport().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::CopyGpuTraceReport) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Fixed-camera GPU A/B: original vs GI -1 vs Shadow -1; each gets a GPU trace and matching image"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Run GPU Pass A/B (GI and Shadow)")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetRunner().IsRunning() && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::RunGpuPassStudy) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open Baseline GPU Trace")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && S->HasGpuPassStudy(); })
                .OnClicked_Lambda([this]() { return OpenGpuPassStudyTrace(0); }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open GI GPU Trace")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && S->HasGpuPassStudy(); })
                .OnClicked_Lambda([this]() { return OpenGpuPassStudyTrace(1); }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open Shadow GPU Trace")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && S->HasGpuPassStudy(); })
                .OnClicked_Lambda([this]() { return OpenGpuPassStudyTrace(2); }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open Baseline vs GI Image")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetGpuPassStudyGIImagePath().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::OpenGpuPassStudyGIImage) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open Baseline vs Shadow Image")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetGpuPassStudyShadowImagePath().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::OpenGpuPassStudyShadowImage) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetGpuTraceText) ]
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SNew(SExpandableArea)
                .AreaTitle(FText::FromString(TEXT("Lumen detail and scene structure")))
                .InitiallyCollapsed(true)
                .BodyContent()
                [ SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Investigate Lumen probe cost, VSM risk candidates, and repeated meshes for instancing review."))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Engine scene diagnosis: VSM risk and repeated mesh/material groups"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Scan VSM and Instancing Candidates")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsFullAnalysisRunning() && !S->GetRunner().IsRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && !S->IsLumenDetailStudyRunning(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::ScanEngineScene) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetEngineSceneText) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Copy Scene Diagnosis Report")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetEngineSceneReport().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::CopyEngineSceneReport) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Run Lumen Screen-Probe A/B")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsFullAnalysisRunning() && !S->GetRunner().IsRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && !S->IsLumenDetailStudyRunning() && !S->IsVisualComparisonRunning(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::RunLumenDetailStudy) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open Lumen Detail A/B Image")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetLumenDetailImagePath().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::OpenLumenDetailImage) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetLumenDetailText) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Copy Lumen Detail Report")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsLumenDetailStudyRunning() && !S->GetLumenDetailReport().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::CopyLumenDetailReport) ]
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SNew(SExpandableArea)
                .AreaTitle(FText::FromString(TEXT("Complete analysis and final decision")))
                .InitiallyCollapsed(true)
                .BodyContent()
                [ SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Runs settings search, GPU pass A/B, scene scan, up to 3 shadow groups, Lumen A/B, and final images. Automatically copies the completed or stopped report. Motion capture is available separately."))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Run All Analyses + Copy Report")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->GetRunner().IsRunning() && !S->IsVisualComparisonRunning() && !S->IsMotionComparisonRunning() && !S->IsShadowStudyRunning() && !S->IsGpuTraceCaptureRunning() && !S->IsLumenDetailStudyRunning(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::RunBenchmark) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Full analysis decision"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(STextBlock).Text(this, &SGraphicsOptimizerPanel::GetFullAnalysisText) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Apply Recommended to PIE")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && S->IsFullAnalysisComplete(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::ApplyFullRecommendation) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Keep Original Settings")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && S->IsFullAnalysisComplete(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::KeepOriginalFullAnalysis) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Open Final Baseline vs Recommendation Image")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && S->IsFullAnalysisComplete() && !S->GetComparisonImagePath().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::OpenComparisonImage) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [ SNew(SButton).Text(FText::FromString(TEXT("Copy Full Analysis Report")))
                .IsEnabled_Lambda([this]() { UGraphicsOptimizerSubsystem* S = GetSubsystem(); return S && !S->IsFullAnalysisRunning() && !S->GetFullAnalysisReport().IsEmpty(); })
                .OnClicked(this, &SGraphicsOptimizerPanel::CopyFullAnalysisReport) ]
                ]
            ]
        ]
    ];
}

FText SGraphicsOptimizerPanel::GetStatUnitText() const
{
    UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
    UGameInstance* GameInstance = PlayWorld ? PlayWorld->GetGameInstance() : nullptr;
    UGameViewportClient* Viewport = GameInstance ? GameInstance->GetGameViewportClient() : nullptr;
    const FStatUnitData* Unit = Viewport ? Viewport->GetStatUnitData() : nullptr;
    if (!Viewport || !Viewport->IsStatEnabled(TEXT("Unit")) || !Unit)
        return FText::FromString(TEXT("Engine stat unit: inactive (enabled automatically during a benchmark)."));
    return FText::FromString(FString::Printf(TEXT("Engine stat unit now (smoothed): Frame %.2f | Game %.2f | Draw %.2f | GPU %.2f ms"),
        Unit->FrameTime, Unit->GameThreadTime, Unit->RenderThreadTime, Unit->GPUFrameTime[0]));
}

FText SGraphicsOptimizerPanel::GetStatusText() const
{
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem) return FText::FromString(TEXT("Start PIE to enable benchmarking."));
    const FExperimentRunner& Runner = Subsystem->GetRunner();
    const Scalability::FQualityLevels Current = Scalability::GetQualityLevels();
    const FString Resolution = Current.ResolutionQuality == 0.0f
        ? (Runner.GetObservedBaselineResolutionScale() > 0.0f
            ? FString::Printf(TEXT("project default (observed primary %.1f%%)"), Runner.GetObservedBaselineResolutionScale())
            : TEXT("project default"))
        : FString::Printf(TEXT("%.0f%%"), Current.ResolutionQuality);
    const FString EffectiveCap = Runner.GetFrameRateCapFPS() > 0.0f
        ? FString::Printf(TEXT("%.1f FPS"), Runner.GetFrameRateCapFPS()) : TEXT("none reported");
    const FString CapWarning = Runner.GetFrameRateCapFPS() > 0.0f &&
        Runner.GetOptions().TargetFPS >= Runner.GetFrameRateCapFPS() - 0.5f
        ? FString(TEXT("\nWarning: target is at or above the engine tick cap; raise the cap before judging target FPS."))
        : FString();
    FString Status = FString::Printf(
        TEXT("State: %s\nGPU: %s\nEffective max tick rate: %s%s\nCurrent: resolution %s, shadow %d, GI %d, reflection %d, post %d, texture %d, effects %d\nApplied selection active: %s\nPreset path: %s%s%s%s%s"),
        StateLabel(Runner.GetState()), *GRHIAdapterName, *EffectiveCap, *CapWarning, *Resolution, Current.ShadowQuality,
        Current.GlobalIlluminationQuality, Current.ReflectionQuality, Current.PostProcessQuality,
        Current.TextureQuality, Current.EffectsQuality, Subsystem->HasAppliedConfiguration() ? TEXT("yes") : TEXT("no"),
        *Subsystem->GetSavedPresetPath(),
        Subsystem->GetLastActionMessage().IsEmpty() ? TEXT("") : TEXT("\nAction: "), *Subsystem->GetLastActionMessage(),
        Runner.GetLastError().IsEmpty() ? TEXT("") : TEXT("\nError: "), *Runner.GetLastError());
    if (Subsystem->IsVisualComparisonRunning()) Status += TEXT("\nA/B capture: running");
    if (Subsystem->IsMotionComparisonRunning()) Status += TEXT("\nA/D motion comparison: running");
    if (Subsystem->IsShadowStudyRunning()) Status += TEXT("\nShadow caster A/B: running");
    if (Subsystem->IsGpuTraceCaptureRunning()) Status += TEXT("\nGPU trace capture: running");
    if (!Subsystem->GetComparisonImagePath().IsEmpty())
        Status += TEXT("\nA/B image (left baseline, right selected): ") + Subsystem->GetComparisonImagePath();
    if (!Subsystem->GetMotionVideoPath().IsEmpty())
        Status += TEXT("\nA/D motion video: ") + Subsystem->GetMotionVideoPath();
    if (!Subsystem->GetShadowStudyImagePath().IsEmpty())
        Status += TEXT("\nShadow A/B image: ") + Subsystem->GetShadowStudyImagePath();
    return FText::FromString(Status);
}

FText SGraphicsOptimizerPanel::GetMotionText() const
{
    const UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    return Subsystem && !Subsystem->GetMotionReport().IsEmpty()
        ? FText::FromString(Subsystem->GetMotionReport())
        : FText::FromString(TEXT("Motion results appear here after the 4-second A/D comparison."));
}

FText SGraphicsOptimizerPanel::GetShadowCandidateText() const
{
    const UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    return Subsystem && !Subsystem->GetShadowCandidateList().IsEmpty()
        ? FText::FromString(Subsystem->GetShadowCandidateList())
        : FText::FromString(TEXT("No shadow scan yet."));
}

FText SGraphicsOptimizerPanel::GetShadowStudyText() const
{
    const UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    return Subsystem && !Subsystem->GetShadowStudyReport().IsEmpty()
        ? FText::FromString(Subsystem->GetShadowStudyReport())
        : FText::FromString(TEXT("Choose a scanned mesh group. The study pauses gameplay, measures the current scene, disables only that group's CastShadow during the second pass, and restores it."));
}

FText SGraphicsOptimizerPanel::GetGpuTraceText() const
{
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem) return FText::GetEmpty();
    if (Subsystem->GetGpuTracePath() != AnalyzedGpuTracePath && !GpuPassAnalysis.IsEmpty())
        return FText::FromString(Subsystem->GetGpuTraceReport());
    return FText::FromString(Subsystem->GetGpuTraceReport() + TEXT("\n") + GpuPassAnalysis);
}

FText SGraphicsOptimizerPanel::GetFullAnalysisText() const
{
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem) return FText::GetEmpty();
    if (Subsystem->IsFullAnalysisRunning())
        return FText::FromString(Subsystem->GetFullAnalysisProgress() +
            TEXT(" Keep PIE focused; results and apply choices appear after every stage finishes."));
    if (Subsystem->GetFullAnalysisReport().IsEmpty()) return FText::GetEmpty();
    const bool bSameRun = !Subsystem->GetGpuTraceDirectory().IsEmpty() &&
        Subsystem->GetFullAnalysisReport().Contains(Subsystem->GetGpuTraceDirectory());
    if (CachedFullAnalysisDirectory != Subsystem->GetFullAnalysisDirectory())
    {
        CachedFullAnalysisDirectory = Subsystem->GetFullAnalysisDirectory();
        CachedFullGpuAnalysis.Empty();
        FFileHelper::LoadFileToString(CachedFullGpuAnalysis,
            *FPaths::Combine(CachedFullAnalysisDirectory, TEXT("FullGpuPassAnalysis.txt")));
    }
    const FString Passes = bSameRun && Subsystem->GetGpuTracePath() == AnalyzedGpuTracePath
        ? GpuPassAnalysis : CachedFullGpuAnalysis;
    return FText::FromString(Subsystem->GetFullAnalysisReport() + TEXT("\n8. GPU pass timer comparison\n") +
        (Passes.IsEmpty() ? TEXT("Analysis pending or unavailable.\n") : Passes));
}

FText SGraphicsOptimizerPanel::GetEngineSceneText() const
{
    const UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    return FText::FromString(Subsystem ? Subsystem->GetEngineSceneReport() : FString());
}

FText SGraphicsOptimizerPanel::GetLumenDetailText() const
{
    const UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    return FText::FromString(Subsystem ? Subsystem->GetLumenDetailReport() : FString());
}

FReply SGraphicsOptimizerPanel::ScanEngineScene()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem()) Subsystem->ScanEngineScene();
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::RunLumenDetailStudy()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem()) Subsystem->StartLumenDetailStudy(MeasurementSeconds);
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::OpenLumenDetailImage()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
        if (!Subsystem->GetLumenDetailImagePath().IsEmpty())
            FPlatformProcess::LaunchFileInDefaultExternalApplication(*Subsystem->GetLumenDetailImagePath());
    return FReply::Handled();
}

const FSlateBrush* SGraphicsOptimizerPanel::GetComparisonBrush() const
{
    const UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    const FString Path = Subsystem ? Subsystem->GetComparisonImagePath() : FString();
    if (Path != CachedComparisonImagePath)
    {
        ComparisonBrush.Reset();
        CachedComparisonImagePath = Path;
        if (!Path.IsEmpty())
        {
            const FIntPoint Size = Subsystem->GetComparisonSize();
            const float Height = Size.X > 0 ? 700.0f * Size.Y / (2.0f * Size.X) : 200.0f;
            ComparisonBrush = MakeShared<FSlateDynamicImageBrush>(FName(*Path), FVector2D(700.0f, Height));
        }
    }
    return ComparisonBrush.Get();
}

const FSlateBrush* SGraphicsOptimizerPanel::GetShadowBrush() const
{
    const UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    const FString Path = Subsystem ? Subsystem->GetShadowStudyImagePath() : FString();
    if (Path != CachedShadowImagePath)
    {
        ShadowBrush.Reset();
        CachedShadowImagePath = Path;
        if (!Path.IsEmpty())
        {
            const FIntPoint Size = Subsystem->GetShadowStudySize();
            const float Height = Size.X > 0 ? 700.0f * Size.Y / (2.0f * Size.X) : 200.0f;
            ShadowBrush = MakeShared<FSlateDynamicImageBrush>(FName(*Path), FVector2D(700.0f, Height));
        }
    }
    return ShadowBrush.Get();
}

FText SGraphicsOptimizerPanel::GetResultsText() const
{
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem) return FText::FromString(TEXT("No active PIE session."));
    FString Lines;
    for (const FGraphicsExperimentResult& Result : Subsystem->GetRunner().GetResults())
    {
        const FGraphicsPerformanceSummary& P = Result.Performance;
        const FString CapLabel = Result.bFPSComparisonLimited
            ? FString::Printf(TEXT(", capped at %.1f FPS"), Result.FrameRateCapFPS) : FString();
        const FString FPSLabel = FMath::Abs(P.AverageFPS - Subsystem->GetRunner().GetOptions().TargetFPS) < 0.01
            ? FString::Printf(TEXT("%.4f"), P.AverageFPS)
            : FString::Printf(TEXT("%.2f"), P.AverageFPS);
        Lines += FString::Printf(TEXT("#%d %s | %s FPS (%+.2f%s) | frame %.2f ms | 1%% low %.1f FPS | game %.2f ms | "),
            Result.ExperimentNumber, *Result.Label, *FPSLabel, Result.FPSDifference, *CapLabel,
            P.AverageFrameMS, P.OnePercentLowFPS, P.AverageGameMS);
        if (P.RenderSamples > 0)
            Lines += FString::Printf(TEXT("render %.2f ms (direct) | "), P.AverageRenderMS);
        else if (P.StatUnitRenderMS >= 0.01)
            Lines += FString::Printf(TEXT("render %.2f ms (stat unit Draw) | "), P.StatUnitRenderMS);
        else
            Lines += TEXT("render unavailable | ");
        Lines += P.RenderCriticalSamples > 0
            ? FString::Printf(TEXT("render critical %.2f ms (includes waits) | "), P.AverageRenderCriticalMS)
            : TEXT("render critical unavailable | ");
        Lines += P.GPUSamples > 0 ? FString::Printf(TEXT("GPU %.2f ms (%d samples)"), P.AverageGPUMS, P.GPUSamples) : TEXT("GPU unavailable");
        if (Result.ObservedPrimaryResolutionScale > 0.0f)
            Lines += FString::Printf(TEXT(" | observed primary %.1f%%"), Result.ObservedPrimaryResolutionScale);
        if (P.LongHitchSamples > 0)
            Lines += FString::Printf(TEXT(" | %d long hitches (max %.0f ms)"), P.LongHitchSamples, P.LongestFrameMS);
        Lines += FString::Printf(TEXT(" | target %s\n"),
            !Result.bTargetEvaluationValid ? TEXT("inconclusive (frame cap)") : Result.bMetTargetFPS ? TEXT("met") : TEXT("missed"));
        if (P.StatUnitSamples > 0)
        {
            Lines += FString::Printf(TEXT("  stat unit avg (%d): Frame %.2f | Game %.2f | Draw %.4f | GPU %.2f ms\n"),
                P.StatUnitSamples, P.StatUnitFrameMS, P.StatUnitGameMS, P.StatUnitRenderMS, P.StatUnitGPUMS);
            Lines += FString::Printf(TEXT("  plugin - stat unit: Frame %+.2f | Game %+.2f | "),
                P.AverageFrameMS - P.StatUnitFrameMS, P.AverageGameMS - P.StatUnitGameMS);
            Lines += P.RenderSamples > 0 && P.StatUnitRenderMS >= 0.01
                ? FString::Printf(TEXT("Draw %+.2f | "), P.AverageRenderMS - P.StatUnitRenderMS)
                : TEXT("Draw unavailable | ");
            Lines += P.GPUSamples > 0 && P.StatUnitGPUMS > 0.0
                ? FString::Printf(TEXT("GPU %+.2f ms\n"), P.AverageGPUMS - P.StatUnitGPUMS)
                : TEXT("GPU unavailable\n");
        }
        else Lines += TEXT("  stat unit avg: unavailable\n");
    }
    return FText::FromString(Lines.IsEmpty() ? TEXT("No results yet.") : Lines);
}

FText SGraphicsOptimizerPanel::GetOptimizationText() const
{
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem) return FText::GetEmpty();
    const FExperimentRunner& Runner = Subsystem->GetRunner();
    if (!Runner.WasOptimizationRequested()) return FText::GetEmpty();
    if (Runner.GetState() == EGraphicsOptimizerState::Failed)
        return FText::FromString(TEXT("Optimization stopped without a suggestion."));
    if (Runner.GetState() != EGraphicsOptimizerState::Completed)
        return FText::FromString(TEXT("Optimization is running; the original settings will be restored."));
    const int32 Index = Runner.GetBestResultIndex();
    if (!Runner.GetResults().IsValidIndex(Index))
        return FText::FromString(TEXT("No valid optimization result."));
    const FGraphicsExperimentResult& Best = Runner.GetResults()[Index];
    const Scalability::FQualityLevels& Q = Best.AppliedConfiguration;
    const FString Resolution = Q.ResolutionQuality == 0.0f
        ? (Best.ObservedPrimaryResolutionScale > 0.0f
            ? FString::Printf(TEXT("project default (observed primary %.1f%%)"), Best.ObservedPrimaryResolutionScale)
            : TEXT("project default"))
        : FString::Printf(TEXT("%.0f%%"), Q.ResolutionQuality);
    const int32 FastestIndex = Runner.GetFastestResultIndex();
    const FGraphicsExperimentResult& Fastest = Runner.GetResults().IsValidIndex(FastestIndex)
        ? Runner.GetResults()[FastestIndex] : Best;
    const FGraphicsValidationResult* SelectedValidation = Runner.GetSelectedValidation();
    const TCHAR* Verdict = Runner.HasMetOptimizationTarget() ? TEXT("Target met") :
        Runner.IsValidationComplete() && Runner.HadTargetBeforeValidation() ? TEXT("Target not repeatably met") :
        !Fastest.bTargetEvaluationValid ? TEXT("Target inconclusive (frame cap)") : TEXT("Target missed");
    FString Text = FString::Printf(
        TEXT("%s; quality-preserving measured candidate: %s | %.2f FPS%s | resolution %s, shadow %d, GI %d, reflection %d, post %d, texture %d, effects %d"),
        Verdict,
        *Best.Label, SelectedValidation ? SelectedValidation->MedianFPS : Best.Performance.AverageFPS,
        SelectedValidation ? TEXT(" (validation median)") : TEXT(""), *Resolution, Q.ShadowQuality,
        Q.GlobalIlluminationQuality, Q.ReflectionQuality, Q.PostProcessQuality, Q.TextureQuality, Q.EffectsQuality);
    if (Fastest.AppliedConfiguration != Best.AppliedConfiguration)
    {
        Text += FString::Printf(TEXT("\nFastest observation: %s | %.2f FPS. Quality-step gain threshold: %.2f FPS."),
            *Fastest.Label, Fastest.Performance.AverageFPS, Runner.GetMinimumUsefulGainFPS());
    }
    for (const FGraphicsValidationResult& Entry : Runner.GetValidationResults())
    {
        if (!Runner.GetResults().IsValidIndex(Entry.SourceResultIndex)) continue;
        Text += FString::Printf(TEXT("\nValidation %s: median %.4f FPS, range %.4f-%.4f FPS, target met %d/%d%s"),
            *Runner.GetResults()[Entry.SourceResultIndex].Label, Entry.MedianFPS, Entry.MinimumFPS,
            Entry.MaximumFPS, Entry.TargetMetRuns, FOptimizationManager::ValidationRunsPerCandidate,
            Entry.bAllTargetEvaluationsValid ? TEXT("") : TEXT(" (frame-cap verdict inconclusive)"));
    }
    Text += TEXT("\nMeasured in this scene only. Benchmark settings were restored; Apply and Save are separate actions.");
    return FText::FromString(Text);
}

int32 SGraphicsOptimizerPanel::GetSelectedResultIndex() const
{
    const UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem) return INDEX_NONE;
    return SelectedResultIndex >= 0 ? SelectedResultIndex : Subsystem->GetRunner().GetBestResultIndex();
}

FText SGraphicsOptimizerPanel::GetSelectedResultText() const
{
    const UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem || !Subsystem->GetRunner().GetResults().IsValidIndex(GetSelectedResultIndex()))
        return FText::FromString(TEXT("No result selected. Run Optimization or enter a result number from Run Benchmark."));
    const FGraphicsExperimentResult& Result = Subsystem->GetRunner().GetResults()[GetSelectedResultIndex()];
    const FString Resolution = Result.AppliedConfiguration.ResolutionQuality == 0.0f
        ? FString(TEXT("project default")) : FString::Printf(TEXT("%.0f%%"), Result.AppliedConfiguration.ResolutionQuality);
    return FText::FromString(FString::Printf(TEXT("Selected #%d: %s | resolution %s, shadow %d, GI %d, reflection %d, post %d, texture %d, effects %d"),
        Result.ExperimentNumber, *Result.Label, *Resolution,
        Result.AppliedConfiguration.ShadowQuality, Result.AppliedConfiguration.GlobalIlluminationQuality,
        Result.AppliedConfiguration.ReflectionQuality, Result.AppliedConfiguration.PostProcessQuality,
        Result.AppliedConfiguration.TextureQuality, Result.AppliedConfiguration.EffectsQuality));
}

FReply SGraphicsOptimizerPanel::RunBenchmark()
{
    return StartRun(true, true);
}

FReply SGraphicsOptimizerPanel::RunOptimization()
{
    return StartRun(true);
}

FReply SGraphicsOptimizerPanel::RunSettingsBenchmark()
{
    return StartRun(false);
}

FReply SGraphicsOptimizerPanel::StartRun(bool bOptimize, bool bFullAnalysis)
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        SelectedResultIndex = -1;
        FGraphicsBenchmarkOptions Options;
        Options.TargetFPS = TargetFPS;
        Options.MeasurementSeconds = MeasurementSeconds;
        Options.bFreezeGameplay = bFreezeGameplay;
        Options.bRunOptimization = bOptimize;
        Options.MinimumUsefulGainFPS = MinimumUsefulGainFPS;
        GpuPassAnalysis.Empty();
        AnalyzedGpuTracePath.Empty();
        LastAutoAnalysisPath.Empty();
        const bool bStarted = bFullAnalysis ? Subsystem->StartFullAnalysis(Options) : Subsystem->StartBenchmark(Options);
        if (bStarted) bCopyFullReportOnFinish = bFullAnalysis;
    }
    return FReply::Handled();
}

void SGraphicsOptimizerPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem || Subsystem->IsFullAnalysisRunning() || Subsystem->GetFullAnalysisReport().IsEmpty()) return;
    const FString& TracePath = Subsystem->GetGpuTracePath();
    if (Subsystem->HasGpuPassStudy() && !TracePath.IsEmpty() && TracePath != LastAutoAnalysisPath &&
        Subsystem->GetFullAnalysisReport().Contains(Subsystem->GetGpuTraceDirectory()))
    {
        LastAutoAnalysisPath = TracePath;
        AnalyzeGpuTrace();
        const FString FullPath = FPaths::Combine(Subsystem->GetFullAnalysisDirectory(), TEXT("FullAnalysisWithGpuPasses.txt"));
        FFileHelper::SaveStringToFile(GetFullAnalysisText().ToString(), *FullPath);
        Subsystem->ArchiveReport(TEXT("Full:") + Subsystem->GetFullAnalysisDirectory(), TEXT("Full analysis"), GetFullAnalysisText().ToString());
    }
    if (bCopyFullReportOnFinish)
    {
        bCopyFullReportOnFinish = false;
        CopyFullAnalysisReport();
        UE_LOG(LogTemp, Log, TEXT("[GraphicsOptimizer][Report] Full analysis report copied to clipboard (completed or stopped)."));
    }
}

FReply SGraphicsOptimizerPanel::ApplyFullRecommendation()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
        if (Subsystem->ApplyFullAnalysisRecommendation())
            Subsystem->ArchiveReport(TEXT("Full:") + Subsystem->GetFullAnalysisDirectory(), TEXT("Full analysis"), GetFullAnalysisText().ToString());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::KeepOriginalFullAnalysis()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        Subsystem->KeepOriginalAfterFullAnalysis();
        if (!Subsystem->GetFullAnalysisReport().IsEmpty())
            Subsystem->ArchiveReport(TEXT("Full:") + Subsystem->GetFullAnalysisDirectory(), TEXT("Full analysis"), GetFullAnalysisText().ToString());
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::CopyFullAnalysisReport()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        if (Subsystem->IsFullAnalysisRunning() || Subsystem->GetFullAnalysisReport().IsEmpty()) return FReply::Handled();
        if (Subsystem->HasGpuPassStudy() && Subsystem->GetGpuTracePath() != AnalyzedGpuTracePath &&
            Subsystem->GetFullAnalysisReport().Contains(Subsystem->GetGpuTraceDirectory())) AnalyzeGpuTrace();
        const FString Report = GetFullAnalysisText().ToString();
        if (!Report.IsEmpty())
        {
            FPlatformApplicationMisc::ClipboardCopy(*Report);
            FFileHelper::SaveStringToFile(Report,
                *FPaths::Combine(Subsystem->GetFullAnalysisDirectory(), TEXT("FullAnalysisWithGpuPasses.txt")));
            Subsystem->ArchiveReport(TEXT("Full:") + Subsystem->GetFullAnalysisDirectory(), TEXT("Full analysis"), Report);
        }
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::CancelBenchmark()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem()) Subsystem->CancelBenchmark();
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::CopySettingsReport()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
        if (!Subsystem->GetLatestBenchmarkReport().IsEmpty())
            FPlatformApplicationMisc::ClipboardCopy(*Subsystem->GetLatestBenchmarkReport());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::CopyEngineSceneReport()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
        if (!Subsystem->GetEngineSceneReport().IsEmpty())
            FPlatformApplicationMisc::ClipboardCopy(*Subsystem->GetEngineSceneReport());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::CopyLumenDetailReport()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
        if (!Subsystem->GetLumenDetailReport().IsEmpty())
            FPlatformApplicationMisc::ClipboardCopy(*Subsystem->GetLumenDetailReport());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::CopyAccumulatedReports()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        if (!Subsystem->IsGpuTraceCaptureRunning() && !Subsystem->GetGpuTraceReport().IsEmpty() &&
            Subsystem->GetGpuTracePath() != AnalyzedGpuTracePath) AnalyzeGpuTrace();
        const FString Report = Subsystem->GetAccumulatedReports();
        FPlatformApplicationMisc::ClipboardCopy(*Report);
        IFileManager::Get().MakeDirectory(*Subsystem->GetReportHistoryDirectory(), true);
        FFileHelper::SaveStringToFile(Report,
            *FPaths::Combine(Subsystem->GetReportHistoryDirectory(), TEXT("AccumulatedReport.txt")));
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::OpenReportHistory()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        const FString Directory = Subsystem->GetReportHistoryDirectory();
        IFileManager::Get().MakeDirectory(*Directory, true);
        FPlatformProcess::ExploreFolder(*Directory);
    }
    return FReply::Handled();
}

void SGraphicsOptimizerPanel::SaveGpuAnalysisHistory()
{
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem || Subsystem->GetGpuTracePath() != AnalyzedGpuTracePath || GpuPassAnalysis.IsEmpty()) return;
    Subsystem->ArchiveReport(TEXT("GPU:") + Subsystem->GetGpuTraceDirectory(), TEXT("GPU study with timer analysis"),
        Subsystem->GetGpuTraceReport() + TEXT("\n") + GpuPassAnalysis);
    if (!Subsystem->GetFullAnalysisReport().IsEmpty() && !Subsystem->GetGpuTraceDirectory().IsEmpty() &&
        Subsystem->GetFullAnalysisReport().Contains(Subsystem->GetGpuTraceDirectory()))
    {
        CachedFullAnalysisDirectory = Subsystem->GetFullAnalysisDirectory();
        CachedFullGpuAnalysis = GpuPassAnalysis;
        FFileHelper::SaveStringToFile(CachedFullGpuAnalysis,
            *FPaths::Combine(CachedFullAnalysisDirectory, TEXT("FullGpuPassAnalysis.txt")));
    }
}

FReply SGraphicsOptimizerPanel::CopyReport()
{
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem) return FReply::Handled();
    const FGraphicsBenchmarkOptions& Run = Subsystem->GetRunner().GetOptions();
    const FString Report = FString::Printf(TEXT("Graphics Optimizer | latest results from possibly separate runs; not cumulative history\nMode: %s\nTarget: %.1f FPS\nMeasurement: %.1f sec\nStatic scene: %s\nMinimum useful gain: %.2f FPS per setting\n"),
        Run.bRunOptimization ? TEXT("Optimization") : TEXT("Benchmark"), Run.TargetFPS,
        Run.MeasurementSeconds, Run.bFreezeGameplay ? TEXT("yes") : TEXT("no"), Run.MinimumUsefulGainFPS)
        + GetStatusText().ToString() + TEXT("\n") + GetSelectedResultText().ToString()
        + TEXT("\n\n") + GetOptimizationText().ToString()
        + TEXT("\n\n") + GetResultsText().ToString()
        + TEXT("\n\n") + GetMotionText().ToString()
        + TEXT("\n\n") + GetShadowStudyText().ToString()
        + TEXT("\n\n") + GetGpuTraceText().ToString()
        + TEXT("\n\n") + GetEngineSceneText().ToString()
        + TEXT("\n\n") + GetLumenDetailText().ToString()
        + TEXT("\n\n") + GetFullAnalysisText().ToString();
    FPlatformApplicationMisc::ClipboardCopy(*Report);
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::ApplySelectedResult()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem()) Subsystem->ApplyResult(GetSelectedResultIndex());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::SaveSelectedPreset()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem()) Subsystem->SaveResultPreset(GetSelectedResultIndex());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::ApplySavedPreset()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem()) Subsystem->ApplySavedPreset();
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::RestoreAppliedSettings()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem()) Subsystem->RestoreAppliedConfiguration();
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::CaptureVisualComparison()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem()) Subsystem->StartVisualComparison(GetSelectedResultIndex());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::OpenComparisonImage()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        if (!Subsystem->GetComparisonImagePath().IsEmpty())
            FPlatformProcess::LaunchFileInDefaultExternalApplication(*Subsystem->GetComparisonImagePath());
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::RunMotionComparison()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem()) Subsystem->StartMotionComparison(GetSelectedResultIndex());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::OpenMotionVideo()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        if (!Subsystem->GetMotionVideoPath().IsEmpty())
            FPlatformProcess::LaunchFileInDefaultExternalApplication(*Subsystem->GetMotionVideoPath());
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::ScanShadowCasters()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        ShadowCandidateIndex = 0;
        Subsystem->ScanShadowCasters();
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::RunShadowStudy()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
        Subsystem->StartShadowStudy(ShadowCandidateIndex, MeasurementSeconds);
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::OpenShadowStudyImage()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        if (!Subsystem->GetShadowStudyImagePath().IsEmpty())
        {
            const FString FullPath = FPaths::ConvertRelativePathToFull(Subsystem->GetShadowStudyImagePath());
            FPlatformProcess::LaunchFileInDefaultExternalApplication(*FullPath);
        }
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::CopyShadowStudyReport()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
        FPlatformApplicationMisc::ClipboardCopy(*Subsystem->GetShadowStudyReport());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::CaptureGpuTrace()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        GpuPassAnalysis.Empty();
        AnalyzedGpuTracePath.Empty();
        Subsystem->StartGpuTraceCapture(MeasurementSeconds, bFreezeGameplay);
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::AnalyzeGpuTrace()
{
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem || Subsystem->IsGpuTraceCaptureRunning()) return FReply::Handled();
    const FString Path = Subsystem->GetGpuTracePath();
    if (IFileManager::Get().FileSize(*Path) <= 0) return FReply::Handled();

    if (Subsystem->HasGpuPassStudy())
    {
        ITraceServicesModule& Module = FModuleManager::LoadModuleChecked<ITraceServicesModule>(TEXT("TraceServices"));
        TSharedPtr<TraceServices::IAnalysisService> Service = Module.GetAnalysisService();
        const TArray<FString>& Paths = Subsystem->GetGpuPassStudyTraces();
        const TArray<FGraphicsPerformanceSummary>& Performance = Subsystem->GetGpuPassStudyPerformance();
        const TCHAR* Labels[] = { TEXT("Baseline"), TEXT("GI -1"), TEXT("Shadow -1") };
        const TCHAR* TimerNames[] = { TEXT("RenderDeferredLighting"), TEXT("LumenScreenProbeGather"), TEXT("ShadowDepths") };
        double TimerMS[3][3] = {};
        bool bTimerFound[3][3] = {};
        uint64 EventCounts[3] = {};
        bool bAllTracesRead = Service.IsValid() && Paths.Num() == 3 && Performance.Num() == 3;
        for (int32 Arm = 0; Arm < 3 && bAllTracesRead; ++Arm)
        {
            TSharedPtr<const TraceServices::IAnalysisSession> Session = Service->Analyze(*Paths[Arm]);
            if (!Session.IsValid()) { bAllTracesRead = false; break; }
            TraceServices::FAnalysisSessionReadScope ReadScope(*Session);
            const TraceServices::ITimingProfilerProvider* Provider = TraceServices::ReadTimingProfilerProvider(*Session);
            if (!Provider) { bAllTracesRead = false; break; }
            const TraceServices::ITimingProfilerTimerReader* TimerReader = nullptr;
            Provider->ReadTimers([&TimerReader](const TraceServices::ITimingProfilerTimerReader& Reader) { TimerReader = &Reader; });
            if (!TimerReader) { bAllTracesRead = false; break; }
            auto Collect = [&](uint32 TimelineIndex)
            {
                Provider->ReadTimeline(TimelineIndex, [&](const TraceServices::ITimingProfilerProvider::Timeline& Timeline)
                {
                    Timeline.EnumerateEvents(0.0, Session->GetDurationSeconds() + 1.0,
                        [&](double StartTime, double EndTime, uint32 Depth, const TraceServices::FTimingProfilerEvent& Event)
                        {
                            const TraceServices::FTimingProfilerTimer* Timer = TimerReader->GetTimer(Event.TimerIndex);
                            if (Timer && Timer->Name && EndTime > StartTime)
                            {
                                ++EventCounts[Arm];
                                for (int32 TimerIndex = 0; TimerIndex < 3; ++TimerIndex)
                                {
                                    if (FCString::Strcmp(Timer->Name, TimerNames[TimerIndex]) == 0)
                                    {
                                        TimerMS[Arm][TimerIndex] += (EndTime - StartTime) * 1000.0;
                                        bTimerFound[Arm][TimerIndex] = true;
                                    }
                                }
                            }
                            return TraceServices::EEventEnumerate::Continue;
                        });
                });
            };
            uint32 TimelineIndex = 0;
            if (Provider->GetGpuTimelineIndex(TimelineIndex)) Collect(TimelineIndex);
            if (Provider->GetGpu2TimelineIndex(TimelineIndex)) Collect(TimelineIndex);
            const int32 FrameCount = FMath::Max(1, Performance[Arm].FrameSamples);
            for (int32 TimerIndex = 0; TimerIndex < 3; ++TimerIndex)
                TimerMS[Arm][TimerIndex] /= FrameCount;
        }
        GpuPassAnalysis = TEXT("Targeted GPU pass A/B analysis (inclusive ms per sampled frame; trace boundaries can differ by one GPU frame).\n");
        if (!bAllTracesRead)
            GpuPassAnalysis += TEXT("One or more trace files could not be analyzed. Open each .utrace in Unreal Insights.\n");
        else
        {
            for (int32 Arm = 0; Arm < 3; ++Arm)
            {
                GpuPassAnalysis += FString::Printf(TEXT("%s | %.2f FPS | GPU %.2f ms | %llu GPU events\n"),
                    Labels[Arm], Performance[Arm].AverageFPS, Performance[Arm].AverageGPUMS, EventCounts[Arm]);
                for (int32 TimerIndex = 0; TimerIndex < 3; ++TimerIndex)
                    GpuPassAnalysis += FString::Printf(TEXT("  %s: %s\n"), TimerNames[TimerIndex],
                        bTimerFound[Arm][TimerIndex] ? *FString::Printf(TEXT("%.2f ms"), TimerMS[Arm][TimerIndex]) : TEXT("unavailable"));
                if (Arm > 0)
                {
                    GpuPassAnalysis += FString::Printf(TEXT("  vs baseline: FPS %+.2f | whole GPU %+.2f ms\n"),
                        Performance[Arm].AverageFPS - Performance[0].AverageFPS,
                        Performance[Arm].AverageGPUMS - Performance[0].AverageGPUMS);
                    for (int32 TimerIndex = 0; TimerIndex < 3; ++TimerIndex)
                        if (bTimerFound[Arm][TimerIndex] && bTimerFound[0][TimerIndex])
                            GpuPassAnalysis += FString::Printf(TEXT("  %s change: %+.2f ms\n"), TimerNames[TimerIndex],
                                TimerMS[Arm][TimerIndex] - TimerMS[0][TimerIndex]);
                }
            }
            GpuPassAnalysis += TEXT("Timer regions can be nested; their times must not be added. Check both saved images for visible quality changes before choosing a setting. A/B measurements are scene-specific and do not automatically apply or save the candidates.\n");
        }
        AnalyzedGpuTracePath = Path;
        const FString AnalysisPath = FPaths::Combine(Subsystem->GetGpuTraceDirectory(), TEXT("GpuPassStudyAnalysis.txt"));
        const bool bSaved = FFileHelper::SaveStringToFile(Subsystem->GetGpuTraceReport() + TEXT("\n") + GpuPassAnalysis, *AnalysisPath);
        SaveGpuAnalysisHistory();
        UE_LOG(LogTemp, Log, TEXT("[GraphicsOptimizer][GpuPassStudy] Analysis %s: %s\n%s"),
            bSaved ? TEXT("saved") : TEXT("could not be saved"), *AnalysisPath, *GpuPassAnalysis);
        return FReply::Handled();
    }

    ITraceServicesModule& Module = FModuleManager::LoadModuleChecked<ITraceServicesModule>(TEXT("TraceServices"));
    TSharedPtr<TraceServices::IAnalysisService> Service = Module.GetAnalysisService();
    TSharedPtr<const TraceServices::IAnalysisSession> Session = Service.IsValid() ? Service->Analyze(*Path) : nullptr;
    if (!Session.IsValid())
    {
        GpuPassAnalysis = TEXT("Could not analyze the trace file. Open it in Unreal Insights and inspect the GPU track.");
        return FReply::Handled();
    }

    struct FPassTotal { FString Name; double InclusiveMS = 0.0; uint64 Calls = 0; };
    TMap<FString, FPassTotal> Totals;
    uint64 GpuEventCount = 0;
    uint64 UnaccountedFrameEvents = 0;
    {
        TraceServices::FAnalysisSessionReadScope ReadScope(*Session);
        const TraceServices::ITimingProfilerProvider* Provider = TraceServices::ReadTimingProfilerProvider(*Session);
        if (Provider)
        {
            const TraceServices::ITimingProfilerTimerReader* TimerReader = nullptr;
            Provider->ReadTimers([&TimerReader](const TraceServices::ITimingProfilerTimerReader& Reader) { TimerReader = &Reader; });
            auto CollectTimeline = [&](uint32 TimelineIndex)
            {
                Provider->ReadTimeline(TimelineIndex, [&](const TraceServices::ITimingProfilerProvider::Timeline& Timeline)
                {
                    Timeline.EnumerateEvents(0.0, Session->GetDurationSeconds() + 1.0,
                        [&](double StartTime, double EndTime, uint32 Depth, const TraceServices::FTimingProfilerEvent& Event)
                        {
                            const TraceServices::FTimingProfilerTimer* Timer = TimerReader ? TimerReader->GetTimer(Event.TimerIndex) : nullptr;
                            if (Timer && Timer->Name && EndTime > StartTime)
                            {
                                ++GpuEventCount;
                                if (FStringView(Timer->Name).StartsWith(TEXT("Unaccounted -")))
                                {
                                    ++UnaccountedFrameEvents;
                                    return TraceServices::EEventEnumerate::Continue;
                                }
                                FPassTotal& Total = Totals.FindOrAdd(FString(Timer->Name));
                                Total.Name = Timer->Name;
                                Total.InclusiveMS += (EndTime - StartTime) * 1000.0;
                                ++Total.Calls;
                            }
                            return TraceServices::EEventEnumerate::Continue;
                        });
                });
            };
            uint32 TimelineIndex = 0;
            if (Provider->GetGpuTimelineIndex(TimelineIndex)) CollectTimeline(TimelineIndex);
            if (Provider->GetGpu2TimelineIndex(TimelineIndex)) CollectTimeline(TimelineIndex);
        }
    }
    TArray<FPassTotal> Sorted;
    Totals.GenerateValueArray(Sorted);
    Sorted.Sort([](const FPassTotal& A, const FPassTotal& B) { return A.InclusiveMS > B.InclusiveMS; });
    GpuPassAnalysis = FString::Printf(TEXT("GPU pass analysis: %llu timing events, %d named timers. %llu per-frame Unaccounted markers omitted from the pass ranking. Inclusive time across the captured GPU timelines. Nested timers overlap; do not add rows together.\n"),
        GpuEventCount, Sorted.Num(), UnaccountedFrameEvents);
    if (Sorted.IsEmpty())
    {
        GpuPassAnalysis += TEXT("No GPU timing events were found. Open the trace in Unreal Insights and check whether the GPU track exists; this may require GPU trace support in the active RHI/build.\n");
    }
    else
    {
        const int32 FrameCount = FMath::Max(1, Subsystem->GetGpuTraceFrameSamples());
        GpuPassAnalysis += TEXT("Largest named GPU timer regions in this capture:\n");
        for (int32 Index = 0; Index < FMath::Min(5, Sorted.Num()); ++Index)
            GpuPassAnalysis += FString::Printf(TEXT("- %s: about %.2f ms per sampled frame (%llu calls)\n"),
                *Sorted[Index].Name, Sorted[Index].InclusiveMS / FrameCount, Sorted[Index].Calls);
        GpuPassAnalysis += TEXT("These are inclusive GPU timer regions, not independent optimization gains. Parent and child timers can overlap.\n\n");
        GpuPassAnalysis += FString::Printf(TEXT("Top GPU timers by inclusive total; ms/frame divides by %d sampled frames (approximate if GPU events lag the capture boundary):\n"), FrameCount);
        for (int32 Index = 0; Index < FMath::Min(20, Sorted.Num()); ++Index)
        {
            const FPassTotal& Pass = Sorted[Index];
            GpuPassAnalysis += FString::Printf(TEXT("%2d. %s | %.2f ms/frame | %.1f ms total | %llu calls\n"),
                Index + 1, *Pass.Name, Pass.InclusiveMS / FrameCount, Pass.InclusiveMS, Pass.Calls);
        }
        GpuPassAnalysis += TEXT("Look for ShadowDepths/VSM, Lumen, Translucency, Niagara or other scene-specific timers in the full Insights GPU track before selecting an optimization experiment.\n");
    }
    AnalyzedGpuTracePath = Path;
    const FString AnalysisPath = FPaths::Combine(Subsystem->GetGpuTraceDirectory(), TEXT("GpuPassAnalysis.txt"));
    const bool bSaved = FFileHelper::SaveStringToFile(Subsystem->GetGpuTraceReport() + TEXT("\n") + GpuPassAnalysis, *AnalysisPath);
    SaveGpuAnalysisHistory();
    UE_LOG(LogTemp, Log, TEXT("[GraphicsOptimizer][GpuTrace] Analysis %s: %s\n%s"),
        bSaved ? TEXT("saved") : TEXT("could not be saved"), *AnalysisPath, *GpuPassAnalysis);
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::OpenGpuTrace()
{
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem || IFileManager::Get().FileSize(*Subsystem->GetGpuTracePath()) <= 0) return FReply::Handled();
    const FString InsightsPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::EngineDir(), TEXT("Binaries"), TEXT("Win64"), TEXT("UnrealInsights.exe")));
    if (IFileManager::Get().FileSize(*InsightsPath) > 0)
    {
        const FString Arguments = FString::Printf(TEXT("-OpenTraceFile=\"%s\""), *Subsystem->GetGpuTracePath());
        FProcHandle Process = FPlatformProcess::CreateProc(*InsightsPath, *Arguments, true, false, false, nullptr, 0, nullptr, nullptr);
        if (Process.IsValid()) FPlatformProcess::CloseProc(Process);
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::CopyGpuTraceReport()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        if (!Subsystem->IsGpuTraceCaptureRunning() && Subsystem->GetGpuTracePath() != AnalyzedGpuTracePath)
            AnalyzeGpuTrace();
        if (!Subsystem->GetGpuTraceReport().IsEmpty())
        {
            const FString Analysis = Subsystem->GetGpuTracePath() == AnalyzedGpuTracePath
                ? GpuPassAnalysis : TEXT("GPU pass analysis pending. Press Analyze GPU Passes, then copy again.");
            const FString Report = Subsystem->GetGpuTraceReport() + TEXT("\n") + Analysis;
            FPlatformApplicationMisc::ClipboardCopy(*Report);
        }
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::RunGpuPassStudy()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
    {
        GpuPassAnalysis.Empty();
        AnalyzedGpuTracePath.Empty();
        Subsystem->StartGpuPassStudy(MeasurementSeconds);
    }
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::OpenGpuPassStudyGIImage()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
        if (!Subsystem->GetGpuPassStudyGIImagePath().IsEmpty())
            FPlatformProcess::LaunchFileInDefaultExternalApplication(*Subsystem->GetGpuPassStudyGIImagePath());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::OpenGpuPassStudyShadowImage()
{
    if (UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem())
        if (!Subsystem->GetGpuPassStudyShadowImagePath().IsEmpty())
            FPlatformProcess::LaunchFileInDefaultExternalApplication(*Subsystem->GetGpuPassStudyShadowImagePath());
    return FReply::Handled();
}

FReply SGraphicsOptimizerPanel::OpenGpuPassStudyTrace(int32 Arm)
{
    UGraphicsOptimizerSubsystem* Subsystem = GetSubsystem();
    if (!Subsystem || !Subsystem->HasGpuPassStudy() || !Subsystem->GetGpuPassStudyTraces().IsValidIndex(Arm))
        return FReply::Handled();
    const FString& TracePath = Subsystem->GetGpuPassStudyTraces()[Arm];
    const FString InsightsPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::EngineDir(), TEXT("Binaries"), TEXT("Win64"), TEXT("UnrealInsights.exe")));
    if (IFileManager::Get().FileSize(*InsightsPath) > 0 && IFileManager::Get().FileSize(*TracePath) > 0)
    {
        const FString Arguments = FString::Printf(TEXT("-OpenTraceFile=\"%s\""), *TracePath);
        FProcHandle Process = FPlatformProcess::CreateProc(*InsightsPath, *Arguments, true, false, false, nullptr, 0, nullptr, nullptr);
        if (Process.IsValid()) FPlatformProcess::CloseProc(Process);
    }
    return FReply::Handled();
}
