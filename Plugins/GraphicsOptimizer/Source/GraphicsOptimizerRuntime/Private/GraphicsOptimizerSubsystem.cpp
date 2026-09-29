#include "GraphicsOptimizerSubsystem.h"
#include "GraphicsOptimizerRuntime.h"
#include "GraphicsResolutionProbe.h"
#include "GraphicsPresetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "ImageUtils.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "HAL/PlatformProcess.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "HAL/IConsoleManager.h"
#include "ProfilingDebugging/TraceAuxiliary.h"
#include "ProfilingDebugging/MiscTrace.h"
#include "Misc/StringBuilder.h"

void UGraphicsOptimizerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReportHistorySessionDirectory = FPaths::Combine(GetReportHistoryDirectory(),
        FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    EndFrameHandle = FCoreDelegates::OnEndFrame.AddUObject(this, &UGraphicsOptimizerSubsystem::OnEndFrame);
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Plugin] Game instance subsystem initialized."));
}

void UGraphicsOptimizerSubsystem::Deinitialize()
{
    FCoreDelegates::OnEndFrame.Remove(EndFrameHandle);
    if (IsLumenDetailStudyRunning()) FinishLumenDetailStudy(false, TEXT("PIE ended during Lumen detail study."));
    if (IsGpuTraceCaptureRunning()) FinishGpuTraceCapture(false, TEXT("PIE ended during GPU trace capture."));
    if (IsShadowStudyRunning()) FinishShadowStudy(false, TEXT("PIE ended during shadow study."));
    if (IsMotionComparisonRunning()) FinishMotionComparison(false, TEXT("PIE ended during motion comparison."));
    if (IsVisualComparisonRunning()) FinishVisualComparison(false, TEXT("PIE ended during visual comparison."));
    if (Runner.IsRunning()) Runner.Abort(TEXT("Game instance ended during benchmark."));
    ArchiveBenchmarkIfFinished();
    RestoreStatUnit();
    RestoreGameplayPause();
    if (IsFullAnalysisRunning()) FinishFullAnalysis(false, TEXT("PIE ended during full analysis."));
    if (bHasAppliedConfiguration) RestoreAppliedConfiguration();
    ResolutionProbe.Reset();
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Plugin] Game instance subsystem deinitialized."));
    Super::Deinitialize();
}

FString UGraphicsOptimizerSubsystem::GetReportHistoryDirectory() const
{
    return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),
        TEXT("GraphicsOptimizer"), TEXT("ReportHistory")));
}

void UGraphicsOptimizerSubsystem::ArchiveReport(const FString& RunKey, const FString& Title, const FString& Report)
{
    if (RunKey.IsEmpty() || Report.IsEmpty()) return;
    FString RecordPath;
    if (const FString* ExistingPath = ReportHistoryPaths.Find(RunKey)) RecordPath = *ExistingPath;
    else
    {
        const FString RecordDirectory = FPaths::Combine(ReportHistorySessionDirectory,
            FString::Printf(TEXT("%04d-"), ReportHistoryPaths.Num() + 1) + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
        if (!IFileManager::Get().MakeDirectory(*RecordDirectory, true))
        {
            UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][History] Could not create %s"), *RecordDirectory);
            return;
        }
        RecordPath = FPaths::Combine(RecordDirectory, TEXT("Report.txt"));
        ReportHistoryPaths.Add(RunKey, RecordPath);
    }
    const FString Snapshot = FString::Printf(TEXT("Report: %s\nRun identity: %s\nUpdated: %s\n\n%s\n"),
        *Title, *RunKey, *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")), *Report);
    if (!FFileHelper::SaveStringToFile(Snapshot, *RecordPath))
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][History] Could not save %s"), *RecordPath);
}

FString UGraphicsOptimizerSubsystem::GetAccumulatedReports() const
{
    TArray<FString> Paths;
    IFileManager::Get().FindFilesRecursive(Paths, *GetReportHistoryDirectory(), TEXT("Report.txt"), true, false);
    Paths.Sort();
    FString Report = FString::Printf(TEXT("Graphics Optimizer | accumulated project reports\nRecords: %d\nDirectory: %s\nIndependent runs may have different scenes, cameras and settings. Their measurements must not be combined into one benchmark. Full-analysis records also repeat their constituent studies.\n"),
        Paths.Num(), *GetReportHistoryDirectory());
    for (const FString& Path : Paths)
    {
        FString Snapshot;
        if (FFileHelper::LoadFileToString(Snapshot, *Path))
            Report += TEXT("\n--------------------\n") + Snapshot;
    }
    return Report;
}

void UGraphicsOptimizerSubsystem::ArchiveBenchmarkIfFinished()
{
    if (!bBenchmarkReportPending || Runner.IsRunning()) return;
    bBenchmarkReportPending = false;
    const FGraphicsBenchmarkOptions& Options = Runner.GetOptions();
    LatestBenchmarkReport = FString::Printf(
        TEXT("Settings benchmark | %s\nTarget %.1f FPS | duration %.1f seconds | static scene %s | tick cap %.1f FPS\nError: %s\nSample counts of zero mean unavailable timing data.\n"),
        Runner.GetState() == EGraphicsOptimizerState::Completed ? TEXT("completed") : TEXT("stopped"),
        Options.TargetFPS, Options.MeasurementSeconds, Options.bFreezeGameplay ? TEXT("yes") : TEXT("no"),
        Runner.GetFrameRateCapFPS(), *Runner.GetLastError());
    LatestBenchmarkReport += BenchmarkReportContext;
    for (const FGraphicsExperimentResult& Result : Runner.GetResults())
    {
        const FGraphicsPerformanceSummary& P = Result.Performance;
        const Scalability::FQualityLevels& Q = Result.AppliedConfiguration;
        LatestBenchmarkReport += FString::Printf(
            TEXT("#%d %s | FPS %.2f (%+.2f) | 1%% low %.2f | frame %.2f ms | game %.2f ms (%d) | render %.2f ms (%d) | render critical %.2f ms (%d) | GPU %.2f ms (%d) | frames %d | target %s\n  resolution %.1f (0=project default), observed %.1f%%, shadow %d, GI %d, reflection %d, post %d, texture %d, effects %d\n  stat unit (%d): Frame %.2f | Game %.2f | Draw %.2f | GPU %.2f ms\n"),
            Result.ExperimentNumber, *Result.Label, P.AverageFPS, Result.FPSDifference,
            P.OnePercentLowFPS, P.AverageFrameMS, P.AverageGameMS, P.GameSamples,
            P.AverageRenderMS, P.RenderSamples, P.AverageRenderCriticalMS, P.RenderCriticalSamples,
            P.AverageGPUMS, P.GPUSamples, P.FrameSamples,
            !Result.bTargetEvaluationValid ? TEXT("inconclusive (cap)") : Result.bMetTargetFPS ? TEXT("met") : TEXT("missed"),
            Q.ResolutionQuality, Result.ObservedPrimaryResolutionScale, Q.ShadowQuality,
            Q.GlobalIlluminationQuality, Q.ReflectionQuality, Q.PostProcessQuality, Q.TextureQuality, Q.EffectsQuality,
            P.StatUnitSamples, P.StatUnitFrameMS, P.StatUnitGameMS, P.StatUnitRenderMS, P.StatUnitGPUMS);
    }
    for (const FGraphicsValidationResult& Entry : Runner.GetValidationResults())
        LatestBenchmarkReport += FString::Printf(TEXT("Validation source #%d: median %.2f | range %.2f-%.2f FPS | target %d/%d | stable %s\n"),
            Entry.SourceResultIndex, Entry.MedianFPS, Entry.MinimumFPS, Entry.MaximumFPS,
            Entry.TargetMetRuns, FOptimizationManager::ValidationRunsPerCandidate, Entry.bStableTargetMet ? TEXT("yes") : TEXT("no"));
    LatestBenchmarkReport += FString::Printf(TEXT("Measured recommendation index: %d\n"), Runner.GetBestResultIndex());
    ArchiveReport(BenchmarkReportId, TEXT("Settings benchmark"), LatestBenchmarkReport);
}

bool UGraphicsOptimizerSubsystem::ScanEngineScene()
{
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World || !World->IsGameWorld() || Runner.IsRunning() || IsShadowStudyRunning() ||
        IsGpuTraceCaptureRunning() || IsLumenDetailStudyRunning() || IsVisualComparisonRunning() ||
        IsMotionComparisonRunning())
    {
        LastActionMessage = TEXT("Engine scene scan needs an idle PIE world.");
        return false;
    }
    struct FRepeatedMeshGroup
    {
        FString Key;
        FString MeshPath;
        int32 Count = 0;
    };
    TArray<FRepeatedMeshGroup> Groups;
    TMap<FString, int32> GroupLookup;
    int32 ShadowLights = 0;
    int32 MovableLights = 0;
    int32 StationaryLights = 0;
    int32 DynamicShadowComponents = 0;
    int32 MovableShadowComponents = 0;
    int32 WPOShadowComponents = 0;
    int32 RepeatedEligibleComponents = 0;
    TArray<FString> LightExamples;
    TArray<FString> WPOExamples;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;
        TArray<ULightComponent*> Lights;
        Actor->GetComponents<ULightComponent>(Lights);
        for (ULightComponent* Light : Lights)
        {
            if (!Light || !Light->IsRegistered() || !Light->IsVisible() || !Light->CastShadows) continue;
            ++ShadowLights;
            if (Light->Mobility == EComponentMobility::Movable) ++MovableLights;
            if (Light->Mobility == EComponentMobility::Stationary) ++StationaryLights;
            if (LightExamples.Num() < 8 && Light->Mobility != EComponentMobility::Static)
                LightExamples.Add(Light->GetPathName());
        }
        TArray<UStaticMeshComponent*> Meshes;
        Actor->GetComponents<UStaticMeshComponent>(Meshes);
        for (UStaticMeshComponent* Component : Meshes)
        {
            if (!Component || !Component->IsRegistered() || !Component->IsVisible() ||
                !Component->GetStaticMesh()) continue;
            bool bWPO = false;
            FString MaterialKey;
            for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
            {
                UMaterialInterface* Material = Component->GetMaterial(Slot);
                MaterialKey += TEXT("|") + (Material ? Material->GetPathName() : TEXT("none"));
                if (Material)
                {
                    const UMaterial* Base = Material->GetBaseMaterial();
                    bWPO |= Base && Base->HasVertexPositionOffsetConnected();
                }
            }
            if (Component->CastShadow && Component->bCastDynamicShadow)
            {
                ++DynamicShadowComponents;
                if (Component->Mobility == EComponentMobility::Movable) ++MovableShadowComponents;
                if (bWPO)
                {
                    ++WPOShadowComponents;
                    if (WPOExamples.Num() < 8) WPOExamples.Add(Component->GetPathName());
                }
            }
            if (Actor->IsA<APawn>() || Component->IsA<UInstancedStaticMeshComponent>() ||
                Component->Mobility != EComponentMobility::Static ||
                Component->IsSimulatingPhysics()) continue;
            ++RepeatedEligibleComponents;
            const FString MeshPath = Component->GetStaticMesh()->GetPathName();
            const FString Key = MeshPath + MaterialKey;
            if (int32* ExistingGroupIndex = GroupLookup.Find(Key)) ++Groups[*ExistingGroupIndex].Count;
            else
            {
                const int32 NewGroupIndex = Groups.Num();
                FRepeatedMeshGroup& Group = Groups.AddDefaulted_GetRef();
                Group.Key = Key;
                Group.MeshPath = MeshPath;
                Group.Count = 1;
                GroupLookup.Add(Key, NewGroupIndex);
            }
        }
    }
    Groups.Sort([](const FRepeatedMeshGroup& A, const FRepeatedMeshGroup& B)
    {
        return A.Count == B.Count ? A.MeshPath < B.MeshPath : A.Count > B.Count;
    });
    EngineSceneReport = FString::Printf(
        TEXT("Engine scene diagnosis | PIE scan\nVSM risk candidates (not measured cache invalidations): %d shadow-casting lights (%d movable, %d stationary); %d dynamic-shadow static-mesh components (%d movable, %d with a base material connected to WPO).\n"),
        ShadowLights, MovableLights, StationaryLights, DynamicShadowComponents,
        MovableShadowComponents, WPOShadowComponents);
    IConsoleVariable* VSMEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Enable"));
    IConsoleVariable* VSMCache = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Cache"));
    EngineSceneReport += FString::Printf(TEXT("Renderer CVars: VSM enable %s | VSM cache %s. If VSM is disabled, this list is only a shadow-caster inventory.\n"),
        VSMEnable ? *FString::FromInt(VSMEnable->GetInt()) : TEXT("unavailable"),
        VSMCache ? *FString::FromInt(VSMCache->GetInt()) : TEXT("unavailable"));
    for (const FString& Path : LightExamples) EngineSceneReport += TEXT("  light: ") + Path + TEXT("\n");
    for (const FString& Path : WPOExamples) EngineSceneReport += TEXT("  WPO caster: ") + Path + TEXT("\n");
    EngineSceneReport += TEXT("These flags identify candidates only. Moving lights/casters or WPO can invalidate VSM pages, but this static scan cannot prove an invalidation or its GPU cost. For manual verification in a moving scene, use the VSM cache visualization and compare GPU traces.\n");
    EngineSceneReport += FString::Printf(TEXT("\nInstancing candidates: %d static, non-physics components; %d mesh/material groups. Top repeated groups:\n"),
        RepeatedEligibleComponents, Groups.Num());
    int32 Listed = 0;
    for (const FRepeatedMeshGroup& Group : Groups)
    {
        if (Group.Count < 2 || Listed >= 10) break;
        EngineSceneReport += FString::Printf(TEXT("  %d components | %s | material signature %s\n"),
            Group.Count, *Group.MeshPath, *Group.Key);
        ++Listed;
    }
    if (Listed == 0) EngineSceneReport += TEXT("  No repeated static mesh/material group found.\n");
    EngineSceneReport += TEXT("ISM/HISM conversion is not performed. Review transforms, collision, per-actor logic, lighting and Nanite use before any conversion; count is not a draw-call or FPS measurement.\n");
    const FString SceneReportDirectory = IsFullAnalysisRunning() && !FullAnalysisDirectory.IsEmpty()
        ? FullAnalysisDirectory
        : FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),
            TEXT("GraphicsOptimizer"), TEXT("SceneDiagnostics")));
    IFileManager::Get().MakeDirectory(*SceneReportDirectory, true);
    EngineSceneReport += TEXT("Report: ") + FPaths::Combine(SceneReportDirectory, TEXT("EngineSceneReport.txt")) + TEXT("\n");
    FFileHelper::SaveStringToFile(EngineSceneReport,
        *FPaths::Combine(SceneReportDirectory, TEXT("EngineSceneReport.txt")));
    ArchiveReport(TEXT("Scene:") + SceneReportDirectory, TEXT("Engine scene diagnosis"), EngineSceneReport);
    LastActionMessage = TEXT("Engine scene scan completed: VSM risk and instancing candidates recorded.");
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][EngineScene] %s\n%s"), *LastActionMessage, *EngineSceneReport);
    return true;
}

bool UGraphicsOptimizerSubsystem::StartLumenDetailStudy(float MeasurementSeconds)
{
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
    FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
    APlayerCameraManager* Camera = World ? UGameplayStatics::GetPlayerCameraManager(World, 0) : nullptr;
    if (!World || !World->IsGameWorld() || !Viewport || !Camera || !FApp::HasFocus() ||
        Runner.IsRunning() || IsGpuTraceCaptureRunning() || IsShadowStudyRunning() ||
        IsVisualComparisonRunning() || IsMotionComparisonRunning() || IsLumenDetailStudyRunning())
    {
        LastActionMessage = TEXT("Lumen study needs an idle, focused PIE viewport and a player camera.");
        return false;
    }
    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"));
    if (!CVar)
    {
        LastActionMessage = TEXT("UE did not expose the Lumen screen-probe downsample CVar.");
        return false;
    }
    const int32 Original = CVar->GetInt();
    if (Original < 4 || Original >= 64 || (CVar->GetFlags() & ECVF_SetByMask) > ECVF_SetByCode)
    {
        LastActionMessage = TEXT("Lumen study skipped: the original factor is outside the effective 4-63 range or has higher-priority CVar control.");
        return false;
    }
    const FIntPoint Size = Viewport->GetSizeXY();
    if (Size.X <= 0 || Size.Y <= 0 || static_cast<int64>(Size.X) * Size.Y > 16000000)
    {
        LastActionMessage = TEXT("Lumen study viewport is invalid or exceeds 16 million pixels.");
        return false;
    }
    bLumenStudyPausedWorld = !World->IsPaused();
    if (bLumenStudyPausedWorld && !UGameplayStatics::SetGamePaused(World, true))
    {
        bLumenStudyPausedWorld = false;
        LastActionMessage = TEXT("Could not pause gameplay for Lumen A/B.");
        return false;
    }
    LumenStudyWorld = World;
    LumenStudyViewport = ViewportClient;
    LumenStudyCVar = CVar;
    LumenStudyOriginalFactor = Original;
    LumenStudyCandidateFactor = FMath::Min(Original * 2, 64);
    LumenStudyQuality = Scalability::GetQualityLevels();
    LumenStudyCameraLocation = Camera->GetCameraLocation();
    LumenStudyCameraRotation = Camera->GetCameraRotation();
    LumenStudySize = Size;
    LumenStudyMeasurementSeconds = FMath::Clamp(MeasurementSeconds, 1.0f, 120.0f);
    LumenBaselinePerformance = FGraphicsPerformanceSummary();
    LumenCandidatePerformance = FGraphicsPerformanceSummary();
    LumenBaselinePixels.Reset();
    LumenStudyReport.Empty();
    LumenStudyImagePath.Empty();
    bLumenStudyCompleted = false;
    LumenStudyDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("GraphicsOptimizer"), TEXT("LumenStudies"),
        FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));
    if (!IFileManager::Get().MakeDirectory(*LumenStudyDirectory, true))
    {
        if (bLumenStudyPausedWorld) UGameplayStatics::SetGamePaused(World, false);
        bLumenStudyPausedWorld = false;
        LumenStudyWorld.Reset();
        LumenStudyViewport.Reset();
        LumenStudyCVar = nullptr;
        LastActionMessage = TEXT("Could not create Lumen A/B output directory.");
        return false;
    }
    LumenStudyStage = ELumenStudyStage::BaselineWarmup;
    LumenStudyBenchmark.BeginWarmup(1.0f);
    LastActionMessage = FString::Printf(TEXT("Lumen A/B started: screen-probe downsample factor %d vs %d."),
        LumenStudyOriginalFactor, LumenStudyCandidateFactor);
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Lumen] %s"), *LastActionMessage);
    return true;
}

bool UGraphicsOptimizerSubsystem::CaptureLumenDetailImage(bool bBaseline)
{
    UGameViewportClient* ViewportClient = LumenStudyViewport.Get();
    FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
    if (!Viewport) return false;
    TArray<FColor> Pixels;
    if (!Viewport->ReadPixels(Pixels) || Pixels.Num() != static_cast<int64>(LumenStudySize.X) * LumenStudySize.Y)
        return false;
    int32 VisiblePixels = 0;
    for (const FColor& Pixel : Pixels)
        if (Pixel.R > 2 || Pixel.G > 2 || Pixel.B > 2) ++VisiblePixels;
    if (VisiblePixels == 0) return false;
    if (bBaseline)
    {
        if (!SaveComparisonImage(FPaths::Combine(LumenStudyDirectory, TEXT("Baseline.png")), Pixels, LumenStudySize)) return false;
        LumenBaselinePixels = MoveTemp(Pixels);
        return true;
    }
    if (LumenBaselinePixels.Num() != Pixels.Num() ||
        !SaveComparisonImage(FPaths::Combine(LumenStudyDirectory, TEXT("Candidate.png")), Pixels, LumenStudySize)) return false;
    const int32 Width = LumenStudySize.X;
    const int32 Height = LumenStudySize.Y;
    TArray<FColor> SideBySide;
    SideBySide.SetNumUninitialized(static_cast<int64>(Width) * Height * 2);
    for (int32 Row = 0; Row < Height; ++Row)
    {
        FMemory::Memcpy(SideBySide.GetData() + static_cast<int64>(Row) * Width * 2,
            LumenBaselinePixels.GetData() + static_cast<int64>(Row) * Width, Width * sizeof(FColor));
        FMemory::Memcpy(SideBySide.GetData() + static_cast<int64>(Row) * Width * 2 + Width,
            Pixels.GetData() + static_cast<int64>(Row) * Width, Width * sizeof(FColor));
    }
    LumenStudyImagePath = FPaths::Combine(LumenStudyDirectory, TEXT("SideBySide.png"));
    return SaveComparisonImage(LumenStudyImagePath, SideBySide, FIntPoint(Width * 2, Height));
}

void UGraphicsOptimizerSubsystem::TickLumenDetailStudy()
{
    UWorld* World = LumenStudyWorld.Get();
    UGameViewportClient* ViewportClient = LumenStudyViewport.Get();
    FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
    APlayerCameraManager* Camera = World ? UGameplayStatics::GetPlayerCameraManager(World, 0) : nullptr;
    const int32 ExpectedFactor = LumenStudyStage == ELumenStudyStage::BaselineWarmup ||
        LumenStudyStage == ELumenStudyStage::BaselineMeasure ? LumenStudyOriginalFactor : LumenStudyCandidateFactor;
    if (!World || !Viewport || !Camera || !FApp::HasFocus() || !World->IsPaused() ||
        Viewport->GetSizeXY() != LumenStudySize || Scalability::GetQualityLevels() != LumenStudyQuality ||
        !LumenStudyCVar || LumenStudyCVar->GetInt() != ExpectedFactor ||
        FVector::DistSquared(LumenStudyCameraLocation, Camera->GetCameraLocation()) > 1.0f ||
        !LumenStudyCameraRotation.Equals(Camera->GetCameraRotation(), 0.1f))
    {
        FinishLumenDetailStudy(false, TEXT("Lumen A/B stopped: focus, camera, pause state, viewport, quality, or CVar changed."));
        return;
    }
    if (LumenStudyStage == ELumenStudyStage::BaselineWarmup || LumenStudyStage == ELumenStudyStage::CandidateWarmup)
    {
        if (LumenStudyBenchmark.IsPeriodComplete())
        {
            LumenStudyStage = LumenStudyStage == ELumenStudyStage::BaselineWarmup
                ? ELumenStudyStage::BaselineMeasure : ELumenStudyStage::CandidateMeasure;
            LumenStudyBenchmark.BeginMeasurement(LumenStudyMeasurementSeconds, ViewportClient);
        }
        return;
    }
    LumenStudyBenchmark.TickMeasurement();
    if (!LumenStudyBenchmark.IsPeriodComplete()) return;
    const FGraphicsPerformanceSummary Summary = LumenStudyBenchmark.FinishMeasurement();
    if (Summary.FrameSamples < 30 || Summary.GPUSamples < 30)
    {
        FinishLumenDetailStudy(false, TEXT("Lumen A/B had fewer than 30 frame or GPU samples."));
        return;
    }
    if (LumenStudyStage == ELumenStudyStage::BaselineMeasure)
    {
        LumenBaselinePerformance = Summary;
        if (!CaptureLumenDetailImage(true))
        {
            FinishLumenDetailStudy(false, TEXT("Could not save a visible Lumen baseline image."));
            return;
        }
        LumenStudyCVar->Set(LumenStudyCandidateFactor, ECVF_SetByCode, FName(TEXT("GraphicsOptimizerLumenStudy")));
        if (LumenStudyCVar->GetInt() != LumenStudyCandidateFactor)
        {
            FinishLumenDetailStudy(false, TEXT("Could not apply the Lumen candidate factor."));
            return;
        }
        LumenStudyStage = ELumenStudyStage::CandidateWarmup;
        LumenStudyBenchmark.BeginWarmup(1.0f);
        return;
    }
    LumenCandidatePerformance = Summary;
    FinishLumenDetailStudy(CaptureLumenDetailImage(false),
        TEXT("Lumen screen-probe detail A/B completed."));
}

void UGraphicsOptimizerSubsystem::FinishLumenDetailStudy(bool bSuccess, const FString& Reason)
{
    if (LumenStudyStage == ELumenStudyStage::None) return;
    if (LumenStudyCVar)
    {
        LumenStudyCVar->Unset(ECVF_SetByCode, FName(TEXT("GraphicsOptimizerLumenStudy")));
        if (LumenStudyCVar->GetInt() != LumenStudyOriginalFactor)
            LumenStudyCVar->Set(LumenStudyOriginalFactor, ECVF_SetByCode);
        bSuccess = bSuccess && LumenStudyCVar->GetInt() == LumenStudyOriginalFactor;
    }
    if (bLumenStudyPausedWorld)
        if (UWorld* World = LumenStudyWorld.Get())
            bSuccess = UGameplayStatics::SetGamePaused(World, false) && bSuccess;
    bLumenStudyCompleted = bSuccess;
    LumenStudyReport = FString::Printf(
        TEXT("Lumen screen-probe detail A/B | %s\nCVar: r.Lumen.ScreenProbeGather.DownsampleFactor (%d baseline -> %d candidate; larger tiles can mean fewer probes; renderer may clamp the effective factor).\nBaseline: %.2f FPS | 1%% low %.2f FPS | GPU %.2f ms (%d samples)\nCandidate: %.2f FPS | 1%% low %.2f FPS | GPU %.2f ms (%d samples)\nChange: FPS %+.2f | GPU %+.2f ms\nImage: %s\nOutput directory: %s\nResult: %s\nOriginal CVar value restored: %s. This changes one Lumen detail control, not the whole GI scalability group. Quality must be judged from the image; this is not automatically applied.\n"),
        bSuccess ? TEXT("completed") : TEXT("stopped"), LumenStudyOriginalFactor, LumenStudyCandidateFactor,
        LumenBaselinePerformance.AverageFPS, LumenBaselinePerformance.OnePercentLowFPS,
        LumenBaselinePerformance.AverageGPUMS, LumenBaselinePerformance.GPUSamples,
        LumenCandidatePerformance.AverageFPS, LumenCandidatePerformance.OnePercentLowFPS,
        LumenCandidatePerformance.AverageGPUMS, LumenCandidatePerformance.GPUSamples,
        LumenCandidatePerformance.AverageFPS - LumenBaselinePerformance.AverageFPS,
        LumenCandidatePerformance.AverageGPUMS - LumenBaselinePerformance.AverageGPUMS,
        *LumenStudyImagePath, *LumenStudyDirectory, *Reason,
        LumenStudyCVar && LumenStudyCVar->GetInt() == LumenStudyOriginalFactor ? TEXT("yes") : TEXT("NO"));
    if (!LumenStudyDirectory.IsEmpty())
        FFileHelper::SaveStringToFile(LumenStudyReport, *FPaths::Combine(LumenStudyDirectory, TEXT("LumenStudyReport.txt")));
    ArchiveReport(TEXT("Lumen:") + LumenStudyDirectory, TEXT("Lumen detail A/B"), LumenStudyReport);
    LumenStudyStage = ELumenStudyStage::None;
    LumenStudyWorld.Reset();
    LumenStudyViewport.Reset();
    LumenStudyCVar = nullptr;
    LumenBaselinePixels.Reset();
    bLumenStudyPausedWorld = false;
    LastActionMessage = Reason;
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Lumen] %s\n%s"), *LastActionMessage, *LumenStudyReport);
}

void UGraphicsOptimizerSubsystem::CancelLumenDetailStudy()
{
    if (IsLumenDetailStudyRunning()) FinishLumenDetailStudy(false, TEXT("Lumen A/B cancelled."));
}

void UGraphicsOptimizerSubsystem::EnsureResolutionProbe()
{
    UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
    FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
    if (Viewport && (!ResolutionProbe.IsValid() || ResolutionProbe->GetViewport() != Viewport))
    {
        ResolutionProbe = FSceneViewExtensions::NewExtension<FGraphicsResolutionProbe>(Viewport);
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Resolution] Observing PIE viewport primary resolution fraction."));
    }
}

bool UGraphicsOptimizerSubsystem::StartBenchmark(const FGraphicsBenchmarkOptions& Options)
{
    LastActionMessage.Empty();
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World || !World->IsGameWorld())
    {
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Benchmark] Start rejected: no active game world."));
        return false;
    }
    if (!FApp::HasFocus())
    {
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Benchmark] Start rejected: application is not focused."));
        return false;
    }
    if (Runner.IsRunning() || IsVisualComparisonRunning() || IsMotionComparisonRunning() || IsShadowStudyRunning() || IsGpuTraceCaptureRunning() || IsLumenDetailStudyRunning()) return false;
    EnsureResolutionProbe();
    FGraphicsBenchmarkOptions RunOptions = Options;
    if (ResolutionProbe.IsValid())
    {
        float ObservedPercentage = 0.0f;
        if (ResolutionProbe->GetStablePrimaryPercentage(ObservedPercentage))
            RunOptions.ObservedBaselineResolutionScale = ObservedPercentage;
    }
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Resolution] Baseline primary screen percentage: %s"),
        RunOptions.ObservedBaselineResolutionScale > 0.0f
            ? *FString::Printf(TEXT("observed %.1f%%"), RunOptions.ObservedBaselineResolutionScale)
            : TEXT("unavailable or unstable"));
    UGameViewportClient* Viewport = GetGameInstance()->GetGameViewportClient();
    if (Viewport)
    {
        StatViewport = Viewport;
        if (!Viewport->IsStatEnabled(TEXT("Unit")))
        {
            GEngine->SetEngineStat(World, Viewport, TEXT("Unit"), true);
            bRestoreStatUnit = Viewport->IsStatEnabled(TEXT("Unit"));
            if (bRestoreStatUnit)
            {
                UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][StatUnit] Enabled stat unit overlay for comparison."));
            }
            else
            {
                UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][StatUnit] Could not enable stat unit overlay."));
            }
        }
        else
        {
            UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][StatUnit] Overlay was already enabled; leaving its state unchanged."));
        }
    }
    else
    {
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][StatUnit] No game viewport; stat unit comparison unavailable."));
    }
    bHasFixedCamera = false;
    if (Options.bFreezeGameplay)
    {
        bPreviousPauseState = World->IsPaused();
        if (!bPreviousPauseState && !UGameplayStatics::SetGamePaused(World, true))
        {
            UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Benchmark] Could not pause gameplay for static-scene benchmark."));
            RestoreStatUnit();
            return false;
        }
        PausedWorld = World;
        bRestoreGameplayPause = true;
        if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(World, 0))
        {
            FixedCameraLocation = Camera->GetCameraLocation();
            FixedCameraRotation = Camera->GetCameraRotation();
            bHasFixedCamera = true;
            UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Benchmark] Static scene: gameplay paused; camera location=%s rotation=%s"),
                *FixedCameraLocation.ToCompactString(), *FixedCameraRotation.ToCompactString());
        }
        else
        {
            UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Benchmark] Static scene: no player camera manager; camera motion cannot be checked."));
        }
    }
    ArchiveBenchmarkIfFinished();
    const bool bStarted = Runner.Start(RunOptions, Viewport);
    if (bStarted)
    {
        BenchmarkReportId = TEXT("Settings:") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
        LatestBenchmarkReport.Empty();
        bBenchmarkReportPending = true;
        BenchmarkReportContext = FString::Printf(TEXT("Run identity: %s\nScene: %s | started: %s\n"),
            *BenchmarkReportId, *World->GetMapName(), *FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")));
        if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(World, 0))
            BenchmarkReportContext += TEXT("Camera at start: ") + Camera->GetCameraLocation().ToCompactString()
                + TEXT(" / ") + Camera->GetCameraRotation().ToCompactString() + TEXT("\n");
        FullAnalysisStage = EFullAnalysisStage::None;
        FullAnalysisReport.Empty();
        ComparisonImagePath.Empty();
        ComparisonDirectory.Empty();
        MotionVideoPath.Empty();
        MotionReport.Empty();
    }
    if (!bStarted) { RestoreStatUnit(); RestoreGameplayPause(); }
    return bStarted;
}

bool UGraphicsOptimizerSubsystem::StartFullAnalysis(const FGraphicsBenchmarkOptions& Options)
{
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World || !World->IsGameWorld() || IsFullAnalysisRunning()) return false;
    FullAnalysisStartingQuality = Scalability::GetQualityLevels();
    FullAnalysisPreviousApplyOriginal = OriginalBeforeApply;
    bFullAnalysisHadAppliedConfiguration = bHasAppliedConfiguration;
    bFullAnalysisPausedWorld = !World->IsPaused();
    if (bFullAnalysisPausedWorld && !UGameplayStatics::SetGamePaused(World, true))
    {
        bFullAnalysisPausedWorld = false;
        return false;
    }
    FullAnalysisWorld = World;
    FGraphicsBenchmarkOptions FullOptions = Options;
    FullOptions.bRunOptimization = true;
    FullOptions.bFreezeGameplay = true;
    if (!StartBenchmark(FullOptions))
    {
        if (bFullAnalysisPausedWorld) UGameplayStatics::SetGamePaused(World, false);
        bFullAnalysisPausedWorld = false;
        FullAnalysisWorld.Reset();
        return false;
    }
    FullAnalysisMeasurementSeconds = FMath::Clamp(FullOptions.MeasurementSeconds, 1.0f, 120.0f);
    FullAnalysisShadowIndex = 0;
    FullAnalysisShadowLimit = 0;
    FullAnalysisShadowObservations.Reset();
    ShadowStudyReport.Empty();
    ShadowStudyImagePath.Empty();
    EngineSceneReport.Empty();
    LumenStudyReport.Empty();
    GpuTraceReport.Empty();
    GpuPassStudyTraces.Reset();
    FullAnalysisDecision = TEXT("Pending: original settings retained until you choose Apply Recommended or Keep Original.");
    FullAnalysisDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("GraphicsOptimizer"), TEXT("FullAnalyses"),
        FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));
    IFileManager::Get().MakeDirectory(*FullAnalysisDirectory, true);
    FullAnalysisReport.Empty();
    FullAnalysisOutcome.Empty();
    FullAnalysisStage = EFullAnalysisStage::Benchmark;
    LastActionMessage = TEXT("Full analysis started: optimization, GPU pass A/B, shadow-caster study, and final image comparison will run in sequence.");
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][FullAnalysis] %s"), *LastActionMessage);
    return true;
}

FString UGraphicsOptimizerSubsystem::GetFullAnalysisProgress() const
{
    switch (FullAnalysisStage)
    {
    case EFullAnalysisStage::Benchmark: return TEXT("1/5: Measuring graphics configurations and validating the candidate.");
    case EFullAnalysisStage::GpuPasses: return TEXT("2/5: Capturing baseline, GI, and Shadow GPU pass traces.");
    case EFullAnalysisStage::Shadow: return FString::Printf(TEXT("3/5: Testing shadow-caster mesh group %d/%d."),
        FullAnalysisShadowIndex + 1, FullAnalysisShadowLimit);
    case EFullAnalysisStage::Lumen: return TEXT("4/5: Testing a Lumen screen-probe detail control.");
    case EFullAnalysisStage::Visual: return TEXT("5/5: Capturing baseline and recommended-setting images.");
    case EFullAnalysisStage::Complete: return TEXT("Completed. Review the comparisons and choose Apply or Keep Original.");
    case EFullAnalysisStage::Failed: return TEXT("Stopped. Review the partial report and the last action message.");
    default: return TEXT("No full analysis in progress.");
    }
}

bool UGraphicsOptimizerSubsystem::ApplyConfiguration(const Scalability::FQualityLevels& Configuration, const FString& Label)
{
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World || !World->IsGameWorld() || Runner.IsRunning() || IsVisualComparisonRunning() || IsMotionComparisonRunning() || IsShadowStudyRunning() || IsGpuTraceCaptureRunning() || IsLumenDetailStudyRunning())
    {
        LastActionMessage = TEXT("Apply requires an active PIE/game world and a completed benchmark.");
        return false;
    }
    const Scalability::FQualityLevels Current = Scalability::GetQualityLevels();
    Scalability::SetQualityLevels(Configuration);
    if (Scalability::GetQualityLevels() != Configuration)
    {
        Scalability::SetQualityLevels(Current);
        LastActionMessage = TEXT("Could not apply the selected scalability configuration exactly.");
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Apply] %s"), *LastActionMessage);
        return false;
    }
    if (!bHasAppliedConfiguration) OriginalBeforeApply = Current;
    bHasAppliedConfiguration = true;
    LastActionMessage = FString::Printf(TEXT("Applied in this PIE/game session: %s. Restore is available; no preset was saved."), *Label);
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Apply] %s"), *LastActionMessage);
    return true;
}

bool UGraphicsOptimizerSubsystem::ApplyResult(int32 ResultIndex)
{
    if (Runner.GetState() != EGraphicsOptimizerState::Completed || !Runner.GetResults().IsValidIndex(ResultIndex))
    {
        LastActionMessage = TEXT("Choose a result from a completed run before applying it.");
        return false;
    }
    const FGraphicsExperimentResult& Result = Runner.GetResults()[ResultIndex];
    return ApplyConfiguration(Result.AppliedConfiguration, Result.Label);
}

bool UGraphicsOptimizerSubsystem::SaveResultPreset(int32 ResultIndex)
{
    if (Runner.GetState() != EGraphicsOptimizerState::Completed || !Runner.GetResults().IsValidIndex(ResultIndex))
    {
        LastActionMessage = TEXT("Choose a result from a completed run before saving it.");
        return false;
    }
    const FGraphicsExperimentResult& Result = Runner.GetResults()[ResultIndex];
    if (!FGraphicsPresetManager::Save(Result.AppliedConfiguration))
    {
        LastActionMessage = TEXT("Could not write the selected preset.");
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Preset] %s"), *LastActionMessage);
        return false;
    }
    LastActionMessage = FString::Printf(TEXT("Saved %s to %s. Saving did not change the current graphics settings."),
        *Result.Label, *FGraphicsPresetManager::GetPath());
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Preset] %s"), *LastActionMessage);
    return true;
}

bool UGraphicsOptimizerSubsystem::StartVisualComparison(int32 ResultIndex)
{
    LastActionMessage.Empty();
    if (Runner.IsRunning() || IsVisualComparisonRunning() || IsMotionComparisonRunning() || IsShadowStudyRunning() || IsGpuTraceCaptureRunning() || IsLumenDetailStudyRunning() || Runner.GetState() != EGraphicsOptimizerState::Completed ||
        !Runner.GetResults().IsValidIndex(0) || !Runner.GetResults().IsValidIndex(ResultIndex))
    {
        LastActionMessage = TEXT("Finish a benchmark and select a measured result before capturing A/B images.");
        return false;
    }
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
    FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
    APlayerCameraManager* Camera = World ? UGameplayStatics::GetPlayerCameraManager(World, 0) : nullptr;
    if (!World || !World->IsGameWorld() || !Viewport || !Camera || !FApp::HasFocus())
    {
        LastActionMessage = TEXT("A/B capture needs a focused PIE viewport and a player camera.");
        return false;
    }
    const FIntPoint Size = Viewport->GetSizeXY();
    if (Size.X <= 0 || Size.Y <= 0 || static_cast<int64>(Size.X) * Size.Y > 16000000)
    {
        LastActionMessage = TEXT("PIE viewport size is invalid or too large for A/B capture (maximum 16 million pixels).");
        return false;
    }
    const bool bWasPaused = World->IsPaused();
    if (!bWasPaused && !UGameplayStatics::SetGamePaused(World, true))
    {
        LastActionMessage = TEXT("Could not pause the scene for matching A/B captures.");
        return false;
    }

    bComparisonPausedWorld = !bWasPaused;
    ComparisonWorld = World;
    ComparisonViewport = ViewportClient;
    ComparisonCameraLocation = Camera->GetCameraLocation();
    ComparisonCameraRotation = Camera->GetCameraRotation();
    ComparisonSize = Size;
    ComparisonBaseline = Runner.GetResults()[0].AppliedConfiguration;
    ComparisonSelected = Runner.GetResults()[ResultIndex].AppliedConfiguration;
    ComparisonBaselinePrimary = Runner.GetResults()[0].ObservedPrimaryResolutionScale;
    ComparisonSelectedPrimary = Runner.GetResults()[ResultIndex].ObservedPrimaryResolutionScale;
    ComparisonResultIndex = ResultIndex;
    BeforeComparison = Scalability::GetQualityLevels();
    ComparisonBaselinePixels.Reset();
    ComparisonBaselinePixelDiagnostics.Empty();
    ComparisonSelectedPixelDiagnostics.Empty();
    ComparisonImagePath.Empty();
    ComparisonDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),
        TEXT("GraphicsOptimizer"), TEXT("Comparisons"),
        FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));

    bComparisonStatUnitWasEnabled = ViewportClient->IsStatEnabled(TEXT("Unit"));
    if (bComparisonStatUnitWasEnabled) GEngine->SetEngineStat(World, ViewportClient, TEXT("Unit"), false);
    if (!IFileManager::Get().MakeDirectory(*ComparisonDirectory, true))
    {
        ComparisonStage = EComparisonStage::Baseline;
        FinishVisualComparison(false, TEXT("Could not create the A/B comparison folder."));
        return false;
    }
    ComparisonStage = EComparisonStage::Baseline;
    Scalability::SetQualityLevels(ComparisonBaseline);
    if (Scalability::GetQualityLevels() != ComparisonBaseline)
    {
        FinishVisualComparison(false, TEXT("Could not apply the baseline configuration for A/B capture."));
        return false;
    }
    ComparisonStageStartSeconds = FPlatformTime::Seconds();
    ComparisonStageFrames = 0;
    LastActionMessage = TEXT("Capturing baseline, then selected settings at the same camera. Please keep PIE visible.");
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Comparison] Starting baseline vs result #%d in %s"),
        ResultIndex, *ComparisonDirectory);
    return true;
}

bool UGraphicsOptimizerSubsystem::SaveComparisonImage(const FString& Path, const TArray<FColor>& Pixels, FIntPoint Size, bool bLog) const
{
    if (Pixels.Num() != static_cast<int64>(Size.X) * Size.Y) return false;
    // Viewport readback alpha is not consistently populated by the RHI.
    // Apply this to every still-image caller, including the Lumen study.
    TArray<FColor> OpaquePixels = Pixels;
    int32 ZeroAlphaPixels = 0;
    int32 VisibleRGBPixels = 0;
    for (FColor& Pixel : OpaquePixels)
    {
        if (Pixel.A == 0) ++ZeroAlphaPixels;
        if (Pixel.R > 2 || Pixel.G > 2 || Pixel.B > 2) ++VisibleRGBPixels;
        Pixel.A = 255;
    }
    TArray64<uint8> Compressed;
    FImageUtils::PNGCompressImageArray(Size.X, Size.Y,
        TArrayView64<const FColor>(OpaquePixels.GetData(), OpaquePixels.Num()), Compressed);
    const bool bSaved = !Compressed.IsEmpty() && FFileHelper::SaveArrayToFile(Compressed, *Path);
    if (bLog)
        UE_LOG(LogGraphicsOptimizer, Log,
            TEXT("[GraphicsOptimizer][Comparison] PNG %s: %dx%d, raw zero-alpha=%d, visible RGB=%d, saved alpha=255, compressed %lld bytes, disk %lld bytes, save=%s"),
            *Path, Size.X, Size.Y, ZeroAlphaPixels, VisibleRGBPixels, static_cast<int64>(Compressed.Num()),
            bSaved ? IFileManager::Get().FileSize(*Path) : static_cast<int64>(-1),
            bSaved ? TEXT("ok") : TEXT("failed"));
    return bSaved;
}

void UGraphicsOptimizerSubsystem::TickVisualComparison()
{
    UWorld* World = ComparisonWorld.Get();
    UGameViewportClient* ViewportClient = ComparisonViewport.Get();
    FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
    APlayerCameraManager* Camera = World ? UGameplayStatics::GetPlayerCameraManager(World, 0) : nullptr;
    if (!World || !World->IsGameWorld() || !Viewport || !Camera || !FApp::HasFocus() ||
        Viewport->GetSizeXY() != ComparisonSize ||
        FVector::DistSquared(ComparisonCameraLocation, Camera->GetCameraLocation()) > 1.0f ||
        !ComparisonCameraRotation.Equals(Camera->GetCameraRotation(), 0.1f))
    {
        FinishVisualComparison(false, TEXT("A/B capture stopped: viewport, focus, or camera changed."));
        return;
    }
    ++ComparisonStageFrames;
    if (ComparisonStageFrames < 8 || FPlatformTime::Seconds() - ComparisonStageStartSeconds < 1.0) return;
    const float ExpectedPrimary = ComparisonStage == EComparisonStage::Baseline
        ? ComparisonBaselinePrimary : ComparisonSelectedPrimary;
    if (ExpectedPrimary > 0.0f)
    {
        float ObservedPrimary = 0.0f;
        if (!ResolutionProbe.IsValid() || !ResolutionProbe->GetStablePrimaryPercentage(ObservedPrimary))
        {
            if (FPlatformTime::Seconds() - ComparisonStageStartSeconds > 5.0)
                FinishVisualComparison(false, TEXT("A/B capture stopped: primary resolution could not be verified."));
            return;
        }
        if (FMath::Abs(ObservedPrimary - ExpectedPrimary) > 1.0f)
        {
            FinishVisualComparison(false, TEXT("A/B capture stopped: observed primary resolution differs from the benchmark result."));
            return;
        }
    }

    TArray<FColor> Pixels;
    if (!Viewport->ReadPixels(Pixels) || Pixels.Num() != static_cast<int64>(ComparisonSize.X) * ComparisonSize.Y)
    {
        FinishVisualComparison(false, TEXT("Could not read the PIE viewport pixels."));
        return;
    }
    const TCHAR* CaptureName = ComparisonStage == EComparisonStage::Baseline ? TEXT("Baseline") : TEXT("Selected");
    const int32 SampleCount = Pixels.Num();
    int32 ZeroAlphaSamples = 0;
    int32 NonBlackSamples = 0;
    uint64 RGBSum = 0;
    for (FColor& Pixel : Pixels)
    {
        if (Pixel.A == 0) ++ZeroAlphaSamples;
        const int32 Brightness = static_cast<int32>(Pixel.R) + Pixel.G + Pixel.B;
        if (Brightness > 24) ++NonBlackSamples;
        RGBSum += Brightness;
        // PIE readback can carry valid RGB with alpha=0; PNG viewers composite that as black.
        Pixel.A = 255;
    }
    const FString PixelDiagnostics = FString::Printf(
        TEXT("%s: %dx%d, %d pixels, raw zero-alpha %d, visible-RGB %d, mean RGB %.1f/255, saved alpha 255"),
        CaptureName, ComparisonSize.X, ComparisonSize.Y, SampleCount, ZeroAlphaSamples,
        NonBlackSamples, SampleCount > 0 ? static_cast<double>(RGBSum) / (SampleCount * 3) : 0.0);
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Comparison] %s"), *PixelDiagnostics);
    if (NonBlackSamples == 0)
    {
        FinishVisualComparison(false, FString::Printf(TEXT("A/B capture stopped: %s contained no visible RGB pixels."), CaptureName));
        return;
    }
    if (ComparisonStage == EComparisonStage::Baseline) ComparisonBaselinePixelDiagnostics = PixelDiagnostics;
    else ComparisonSelectedPixelDiagnostics = PixelDiagnostics;
    if (ComparisonStage == EComparisonStage::Baseline)
    {
        ComparisonBaselinePixels = MoveTemp(Pixels);
        if (!SaveComparisonImage(FPaths::Combine(ComparisonDirectory, TEXT("Baseline.png")),
            ComparisonBaselinePixels, ComparisonSize))
        {
            FinishVisualComparison(false, TEXT("Could not save Baseline.png."));
            return;
        }
        ComparisonStage = EComparisonStage::Selected;
        Scalability::SetQualityLevels(ComparisonSelected);
        if (Scalability::GetQualityLevels() != ComparisonSelected)
        {
            FinishVisualComparison(false, TEXT("Could not apply the selected configuration for A/B capture."));
            return;
        }
        ComparisonStageStartSeconds = FPlatformTime::Seconds();
        ComparisonStageFrames = 0;
        LastActionMessage = TEXT("Baseline saved; capturing the selected settings at the same camera.");
        return;
    }
    if (!SaveComparisonImage(FPaths::Combine(ComparisonDirectory, TEXT("Selected.png")), Pixels, ComparisonSize))
    {
        FinishVisualComparison(false, TEXT("Could not save Selected.png."));
        return;
    }
    const int32 Width = ComparisonSize.X;
    const int32 Height = ComparisonSize.Y;
    TArray<FColor> SideBySide;
    SideBySide.SetNumUninitialized(static_cast<int64>(Width) * Height * 2);
    for (int32 Row = 0; Row < Height; ++Row)
    {
        FMemory::Memcpy(SideBySide.GetData() + static_cast<int64>(Row) * Width * 2,
            ComparisonBaselinePixels.GetData() + static_cast<int64>(Row) * Width, Width * sizeof(FColor));
        FMemory::Memcpy(SideBySide.GetData() + static_cast<int64>(Row) * Width * 2 + Width,
            Pixels.GetData() + static_cast<int64>(Row) * Width, Width * sizeof(FColor));
    }
    const FString CombinedPath = FPaths::Combine(ComparisonDirectory, TEXT("SideBySide.png"));
    if (!SaveComparisonImage(CombinedPath, SideBySide, FIntPoint(Width * 2, Height)))
    {
        FinishVisualComparison(false, TEXT("Could not save SideBySide.png."));
        return;
    }
    const FString Notes = FString::Printf(
        TEXT("Left: baseline (#0)\nRight: selected result (#%d): %s\nBaseline: resolution %.1f, shadow %d, GI %d, reflection %d, post %d, texture %d, effects %d\nSelected: resolution %.1f, shadow %d, GI %d, reflection %d, post %d, texture %d, effects %d\nCamera location: %s\nCamera rotation: %s\nViewport: %d x %d\nPixel diagnostics before alpha correction:\n%s\n%s\nResolution 0 means project default. Captures are static PIE frames after a one-second settle; they are not an image-quality score.\n"),
        ComparisonResultIndex, *Runner.GetResults()[ComparisonResultIndex].Label,
        ComparisonBaseline.ResolutionQuality, ComparisonBaseline.ShadowQuality,
        ComparisonBaseline.GlobalIlluminationQuality, ComparisonBaseline.ReflectionQuality,
        ComparisonBaseline.PostProcessQuality, ComparisonBaseline.TextureQuality, ComparisonBaseline.EffectsQuality,
        ComparisonSelected.ResolutionQuality, ComparisonSelected.ShadowQuality,
        ComparisonSelected.GlobalIlluminationQuality, ComparisonSelected.ReflectionQuality,
        ComparisonSelected.PostProcessQuality, ComparisonSelected.TextureQuality, ComparisonSelected.EffectsQuality,
        *ComparisonCameraLocation.ToCompactString(), *ComparisonCameraRotation.ToCompactString(), Width, Height,
        *ComparisonBaselinePixelDiagnostics, *ComparisonSelectedPixelDiagnostics);
    FFileHelper::SaveStringToFile(Notes, *FPaths::Combine(ComparisonDirectory, TEXT("Comparison.txt")));
    ComparisonImagePath = CombinedPath;
    FinishVisualComparison(true, TEXT("A/B images saved. Left is baseline; right is selected. Open the image for full-size inspection."));
}

void UGraphicsOptimizerSubsystem::FinishVisualComparison(bool bSuccess, const FString& Message)
{
    Scalability::SetQualityLevels(BeforeComparison);
    const bool bRestored = Scalability::GetQualityLevels() == BeforeComparison;
    if (bComparisonStatUnitWasEnabled)
    {
        if (UGameViewportClient* Viewport = ComparisonViewport.Get())
            GEngine->SetEngineStat(Viewport->GetWorld(), Viewport, TEXT("Unit"), true);
    }
    if (bComparisonPausedWorld)
    {
        if (UWorld* World = ComparisonWorld.Get()) UGameplayStatics::SetGamePaused(World, false);
    }
    ComparisonStage = EComparisonStage::None;
    ComparisonWorld.Reset();
    ComparisonViewport.Reset();
    ComparisonBaselinePixels.Reset();
    bComparisonPausedWorld = false;
    bComparisonStatUnitWasEnabled = false;
    if (!bSuccess) ComparisonImagePath.Empty();
    LastActionMessage = bRestored ? Message : Message + TEXT(" WARNING: pre-capture graphics settings could not be restored exactly.");
    FString ImageReport;
    FFileHelper::LoadFileToString(ImageReport, *FPaths::Combine(ComparisonDirectory, TEXT("Comparison.txt")));
    ArchiveReport(TEXT("Visual:") + ComparisonDirectory, TEXT("Still image comparison"),
        (bSuccess ? FString(TEXT("completed\n")) : FString(TEXT("stopped\n"))) + LastActionMessage
        + TEXT("\nImage: ") + ComparisonImagePath + TEXT("\n") + ImageReport);
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Comparison] %s%s"),
        bSuccess ? TEXT("Completed: ") : TEXT("Stopped: "), *LastActionMessage);
}

void UGraphicsOptimizerSubsystem::CancelVisualComparison()
{
    if (IsVisualComparisonRunning()) FinishVisualComparison(false, TEXT("A/B capture cancelled."));
}

bool UGraphicsOptimizerSubsystem::StartMotionComparison(int32 ResultIndex)
{
    LastActionMessage.Empty();
    if (Runner.IsRunning() || IsVisualComparisonRunning() || IsMotionComparisonRunning() || IsShadowStudyRunning() || IsGpuTraceCaptureRunning() || IsLumenDetailStudyRunning() ||
        Runner.GetState() != EGraphicsOptimizerState::Completed ||
        !Runner.GetResults().IsValidIndex(0) || !Runner.GetResults().IsValidIndex(ResultIndex))
    {
        LastActionMessage = TEXT("Finish optimization and select a measured result before motion comparison.");
        return false;
    }
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
    APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
    FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
    if (!World || !World->IsGameWorld() || World->IsPaused() || !Viewport || !Controller || !Pawn || !FApp::HasFocus())
    {
        LastActionMessage = TEXT("Motion comparison needs unpaused, focused PIE with a possessed character and visible viewport.");
        return false;
    }
    MotionSize = Viewport->GetSizeXY();
    if (MotionSize.X <= 0 || MotionSize.Y <= 0 || static_cast<int64>(MotionSize.X) * MotionSize.Y > 2100000)
    {
        LastActionMessage = TEXT("Resize the PIE viewport to at most 2.1 million pixels before recording the motion video.");
        return false;
    }
    MotionWorld = World;
    MotionViewport = ViewportClient;
    MotionController = Controller;
    MotionPawn = Pawn;
    MotionStartTransform = Pawn->GetActorTransform();
    MotionStartControlRotation = Controller->GetControlRotation();
    BeforeMotion = Scalability::GetQualityLevels();
    MotionBaseline = Runner.GetResults()[0].AppliedConfiguration;
    MotionSelected = Runner.GetResults()[ResultIndex].AppliedConfiguration;
    MotionResultIndex = ResultIndex;
    MotionBaselinePositions.Reset();
    MotionSelectedPositions.Reset();
    MotionVideoPath.Empty();
    MotionReport.Empty();
    MotionDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),
        TEXT("GraphicsOptimizer"), TEXT("MotionComparisons"),
        FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));
    if (!IFileManager::Get().MakeDirectory(*MotionDirectory, true))
    {
        LastActionMessage = TEXT("Could not create the motion comparison folder.");
        return false;
    }
    bMotionStatUnitWasEnabled = ViewportClient->IsStatEnabled(TEXT("Unit"));
    if (!bMotionStatUnitWasEnabled) GEngine->SetEngineStat(World, ViewportClient, TEXT("Unit"), true);
    BeginMotionStage(EMotionStage::BaselinePerformance);
    if (!IsMotionComparisonRunning()) return false;
    LastActionMessage = TEXT("Motion comparison: 4-second D/A movement under each setting, then video capture. Keep PIE visible.");
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Motion] Started result #%d; output %s"),
        ResultIndex, *MotionDirectory);
    return true;
}

void UGraphicsOptimizerSubsystem::SetMotionInput(int32 Direction)
{
    if (MotionHeldDirection == Direction) return;
    if (APlayerController* Controller = MotionController.Get())
    {
        if (MotionHeldDirection != 0)
            Controller->InputKey(FInputKeyParams(MotionHeldDirection > 0 ? EKeys::D : EKeys::A, IE_Released, 0.0));
        if (Direction != 0)
        {
            const bool bHandled = Controller->InputKey(FInputKeyParams(Direction > 0 ? EKeys::D : EKeys::A, IE_Pressed, 1.0));
            UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Motion] Injected %s pressed; handled=%s."),
                Direction > 0 ? TEXT("D") : TEXT("A"), bHandled ? TEXT("yes") : TEXT("no"));
        }
    }
    MotionHeldDirection = Direction;
}

void UGraphicsOptimizerSubsystem::BeginMotionStage(EMotionStage NextStage)
{
    SetMotionInput(0);
    APlayerController* Controller = MotionController.Get();
    APawn* Pawn = MotionPawn.Get();
    UWorld* World = MotionWorld.Get();
    if (!Controller || !Pawn || !World)
    {
        FinishMotionComparison(false, TEXT("Character or world disappeared during motion comparison."));
        return;
    }
    MotionStage = NextStage;
    const bool bSelected = NextStage == EMotionStage::SelectedPerformance || NextStage == EMotionStage::SelectedVideo;
    Scalability::SetQualityLevels(bSelected ? MotionSelected : MotionBaseline);
    if (Scalability::GetQualityLevels() != (bSelected ? MotionSelected : MotionBaseline))
    {
        FinishMotionComparison(false, TEXT("Could not apply the requested motion comparison settings."));
        return;
    }
    if (ACharacter* Character = Cast<ACharacter>(Pawn))
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement()) Movement->StopMovementImmediately();
    Pawn->SetActorTransform(MotionStartTransform, false, nullptr, ETeleportType::TeleportPhysics);
    Controller->SetControlRotation(MotionStartControlRotation);
    if (FVector::DistSquared(Pawn->GetActorLocation(), MotionStartTransform.GetLocation()) > 4.0f)
    {
        FinishMotionComparison(false, TEXT("Character could not be reset to the recorded start location."));
        return;
    }
    if (NextStage == EMotionStage::BaselinePerformance) MotionBaselinePositions.Add(Pawn->GetActorLocation());
    if (NextStage == EMotionStage::SelectedPerformance) MotionSelectedPositions.Add(Pawn->GetActorLocation());
    UGameViewportClient* Viewport = MotionViewport.Get();
    const bool bVideo = NextStage == EMotionStage::BaselineVideo || NextStage == EMotionStage::SelectedVideo;
    if (Viewport && Viewport->IsStatEnabled(TEXT("Unit")) != !bVideo)
        GEngine->SetEngineStat(World, Viewport, TEXT("Unit"), !bVideo);
    bMotionWarmup = true;
    MotionStageStartSeconds = FPlatformTime::Seconds();
    MotionFrameIndex = 0;
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Motion] Stage %s; starting 1-second camera/renderer settle."),
        NextStage == EMotionStage::BaselinePerformance ? TEXT("baseline FPS") :
        NextStage == EMotionStage::SelectedPerformance ? TEXT("selected FPS") :
        NextStage == EMotionStage::BaselineVideo ? TEXT("baseline video") : TEXT("selected video"));
}

bool UGraphicsOptimizerSubsystem::SaveMotionFramesThrough(int32 LastFrameIndex)
{
    UGameViewportClient* Client = MotionViewport.Get();
    FViewport* Viewport = Client ? Client->Viewport : nullptr;
    if (!Viewport || Viewport->GetSizeXY() != MotionSize) return false;
    TArray<FColor> Pixels;
    if (!Viewport->ReadPixels(Pixels) || Pixels.Num() != static_cast<int64>(MotionSize.X) * MotionSize.Y) return false;
    int32 VisiblePixels = 0;
    for (FColor& Pixel : Pixels)
    {
        if (static_cast<int32>(Pixel.R) + Pixel.G + Pixel.B > 24) ++VisiblePixels;
        Pixel.A = 255;
    }
    if (VisiblePixels == 0) return false;
    TArray64<uint8> Compressed;
    FImageUtils::PNGCompressImageArray(MotionSize.X, MotionSize.Y,
        TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Compressed);
    if (Compressed.IsEmpty()) return false;
    const TCHAR* Prefix = MotionStage == EMotionStage::BaselineVideo ? TEXT("Baseline") : TEXT("Selected");
    while (MotionFrameIndex <= LastFrameIndex)
    {
        const FString Path = FPaths::Combine(MotionDirectory,
            FString::Printf(TEXT("%s_%03d.png"), Prefix, MotionFrameIndex));
        if (!FFileHelper::SaveArrayToFile(Compressed, *Path)) return false;
        ++MotionFrameIndex;
    }
    return true;
}

void UGraphicsOptimizerSubsystem::TickMotionComparison()
{
    UWorld* World = MotionWorld.Get();
    UGameViewportClient* Client = MotionViewport.Get();
    APlayerController* Controller = MotionController.Get();
    APawn* Pawn = MotionPawn.Get();
    if (!World || !World->IsGameWorld() || World->IsPaused() || !Client || !Client->Viewport ||
        Client->Viewport->GetSizeXY() != MotionSize || !Controller || !Pawn ||
        Controller->GetPawn() != Pawn || !FApp::HasFocus())
    {
        FinishMotionComparison(false, TEXT("Motion comparison stopped: PIE focus, viewport, world, or character changed."));
        return;
    }
    const double Now = FPlatformTime::Seconds();
    if (bMotionWarmup)
    {
        if (Now - MotionStageStartSeconds < 1.0) return;
        bMotionWarmup = false;
        MotionRunStartSeconds = Now;
        if (MotionStage == EMotionStage::BaselinePerformance || MotionStage == EMotionStage::SelectedPerformance)
            MotionBenchmark.BeginMeasurement(4.0f, Client);
        SetMotionInput(1);
        return;
    }
    const double Elapsed = Now - MotionRunStartSeconds;
    SetMotionInput(Elapsed < 2.0 ? 1 : Elapsed < 4.0 ? -1 : 0);
    const bool bVideo = MotionStage == EMotionStage::BaselineVideo || MotionStage == EMotionStage::SelectedVideo;
    if (bVideo)
    {
        const int32 LastFrame = FMath::Clamp(FMath::FloorToInt(Elapsed * 24.0), 0, 95);
        if (MotionFrameIndex <= LastFrame && !SaveMotionFramesThrough(LastFrame))
        {
            FinishMotionComparison(false, TEXT("Could not read or save a visible motion video frame."));
            return;
        }
    }
    else
    {
        MotionBenchmark.TickMeasurement();
        TArray<FVector>& Positions = MotionStage == EMotionStage::BaselinePerformance
            ? MotionBaselinePositions : MotionSelectedPositions;
        while (Positions.Num() <= FMath::Min(4, FMath::FloorToInt(Elapsed))) Positions.Add(Pawn->GetActorLocation());
    }
    if (Elapsed < 4.0) return;
    SetMotionInput(0);
    if (MotionStage == EMotionStage::BaselinePerformance)
    {
        MotionBaselinePerformance = MotionBenchmark.FinishMeasurement();
        if (MotionBaselinePositions.Num() < 2 ||
            FVector::Dist(MotionBaselinePositions[0], MotionBaselinePositions[1]) < 10.0f)
        {
            FinishMotionComparison(false, TEXT("A/D input did not move the character at least 10 cm in the first second. Check the active input mapping or starting obstruction."));
            return;
        }
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Motion] Baseline: %.2f FPS, 1%% low %.2f, GPU %.2f ms (%d frames)."),
            MotionBaselinePerformance.AverageFPS, MotionBaselinePerformance.OnePercentLowFPS,
            MotionBaselinePerformance.AverageGPUMS, MotionBaselinePerformance.FrameSamples);
        BeginMotionStage(EMotionStage::SelectedPerformance);
    }
    else if (MotionStage == EMotionStage::SelectedPerformance)
    {
        MotionSelectedPerformance = MotionBenchmark.FinishMeasurement();
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Motion] Selected: %.2f FPS, 1%% low %.2f, GPU %.2f ms (%d frames)."),
            MotionSelectedPerformance.AverageFPS, MotionSelectedPerformance.OnePercentLowFPS,
            MotionSelectedPerformance.AverageGPUMS, MotionSelectedPerformance.FrameSamples);
        BeginMotionStage(EMotionStage::BaselineVideo);
    }
    else if (MotionStage == EMotionStage::BaselineVideo)
    {
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Motion] Baseline video: %d/96 PNG frames."), MotionFrameIndex);
        BeginMotionStage(EMotionStage::SelectedVideo);
    }
    else
    {
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Motion] Selected video: %d/96 PNG frames."), MotionFrameIndex);
        FinishMotionComparison(true, TEXT("Motion comparison captured."));
    }
}

void UGraphicsOptimizerSubsystem::FinishMotionComparison(bool bSuccess, const FString& Message)
{
    SetMotionInput(0);
    Scalability::SetQualityLevels(BeforeMotion);
    const bool bSettingsRestored = Scalability::GetQualityLevels() == BeforeMotion;
    if (ACharacter* Character = Cast<ACharacter>(MotionPawn.Get()))
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement()) Movement->StopMovementImmediately();
    if (APawn* Pawn = MotionPawn.Get()) Pawn->SetActorTransform(MotionStartTransform, false, nullptr, ETeleportType::TeleportPhysics);
    if (APlayerController* Controller = MotionController.Get()) Controller->SetControlRotation(MotionStartControlRotation);
    if (UGameViewportClient* Client = MotionViewport.Get())
    {
        if (Client->IsStatEnabled(TEXT("Unit")) != bMotionStatUnitWasEnabled)
            GEngine->SetEngineStat(Client->GetWorld(), Client, TEXT("Unit"), bMotionStatUnitWasEnabled);
    }
    MotionStage = EMotionStage::None;
    if (bSuccess)
    {
        double MaxPathDifference = 0.0;
        for (int32 Index = 0; Index < FMath::Min(MotionBaselinePositions.Num(), MotionSelectedPositions.Num()); ++Index)
            MaxPathDifference = FMath::Max(MaxPathDifference,
                FVector::Dist(MotionBaselinePositions[Index], MotionSelectedPositions[Index]));
        MotionReport = FString::Printf(
            TEXT("Motion A/B: 4 seconds, D for 2 seconds then A for 2 seconds; 1-second settle per pass.\nResult #%d: %s\nBaseline: %.2f FPS, 1%% low %.2f FPS, frame %.2f ms, GPU %.2f ms (%d frame samples)\nSelected: %.2f FPS, 1%% low %.2f FPS, frame %.2f ms, GPU %.2f ms (%d frame samples)\nSelected - baseline: %+.2f FPS\nMaximum sampled character-path difference: %.1f cm (at 0/1/2/3/4 sec)\nVideo: 24 FPS playback, left baseline/right selected. Overlaid average FPS values come from separate passes without frame recording.\n"),
            MotionResultIndex, *Runner.GetResults()[MotionResultIndex].Label,
            MotionBaselinePerformance.AverageFPS, MotionBaselinePerformance.OnePercentLowFPS,
            MotionBaselinePerformance.AverageFrameMS, MotionBaselinePerformance.AverageGPUMS,
            MotionBaselinePerformance.FrameSamples,
            MotionSelectedPerformance.AverageFPS, MotionSelectedPerformance.OnePercentLowFPS,
            MotionSelectedPerformance.AverageFrameMS, MotionSelectedPerformance.AverageGPUMS,
            MotionSelectedPerformance.FrameSamples,
            MotionSelectedPerformance.AverageFPS - MotionBaselinePerformance.AverageFPS,
            MaxPathDifference);
        MotionReport += TEXT("Output directory: ") + MotionDirectory + TEXT("\n");
        if (MaxPathDifference > 50.0f)
            MotionReport += TEXT("Warning: character paths differed by more than 50 cm; compare FPS with caution.\n");
        if (Runner.GetFrameRateCapFPS() > 0.0f)
        {
            MotionReport += FString::Printf(TEXT("Effective tick cap from optimization: %.1f FPS.\n"),
                Runner.GetFrameRateCapFPS());
            if (MotionSelectedPerformance.AverageFPS >= Runner.GetFrameRateCapFPS() * 0.98)
                MotionReport += TEXT("Selected FPS is near the cap; uncapped performance remains unknown.\n");
        }
        const FString ReportPath = FPaths::Combine(MotionDirectory, TEXT("MotionReport.txt"));
        FFileHelper::SaveStringToFile(MotionReport, *ReportPath);
        const FString OutputPath = FPaths::Combine(MotionDirectory, TEXT("MotionAB.mp4"));
        const FString Args = FString::Printf(
            TEXT("-y -hide_banner -loglevel error -framerate 24 -start_number 0 -i \"%s\" -framerate 24 -start_number 0 -i \"%s\" -filter_complex \"[0:v][1:v]hstack=inputs=2,pad=ceil(iw/2)*2:ceil(ih/2)*2,drawtext=fontfile=C\\\\:/Windows/Fonts/arial.ttf:text='Baseline measured avg %.2f FPS':expansion=none:fontcolor=white:fontsize=24:box=1:boxcolor=black@0.65:boxborderw=8:x=24:y=20,drawtext=fontfile=C\\\\:/Windows/Fonts/arial.ttf:text='Selected measured avg %.2f FPS':expansion=none:fontcolor=white:fontsize=24:box=1:boxcolor=black@0.65:boxborderw=8:x=w/2+24:y=20[v]\" -map \"[v]\" -c:v libx264 -pix_fmt yuv420p -r 24 -frames:v 96 \"%s\""),
            *FPaths::Combine(MotionDirectory, TEXT("Baseline_%03d.png")),
            *FPaths::Combine(MotionDirectory, TEXT("Selected_%03d.png")),
            MotionBaselinePerformance.AverageFPS, MotionSelectedPerformance.AverageFPS, *OutputPath);
        int32 ReturnCode = -1;
        FString StdOut, StdErr;
        const bool bEncoderStarted = FPlatformProcess::ExecProcess(TEXT("ffmpeg.exe"), *Args, &ReturnCode, &StdOut, &StdErr);
        const bool bVideoSaved = bEncoderStarted && ReturnCode == 0 && IFileManager::Get().FileSize(*OutputPath) > 0;
        if (bVideoSaved) MotionVideoPath = OutputPath;
        LastActionMessage = bVideoSaved
            ? FString::Printf(TEXT("4-second motion A/B saved to %s. FPS report: %s"), *OutputPath, *ReportPath)
            : FString::Printf(TEXT("Motion FPS and PNG frames saved to %s, but MP4 encoding failed (code %d). ffmpeg.exe must be on PATH. %s"),
                *MotionDirectory, ReturnCode, *StdErr.Right(300));
        if (bVideoSaved)
        {
            UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Motion] %s"), *LastActionMessage);
        }
        else
        {
            UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Motion] %s"), *LastActionMessage);
        }
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Motion] %s"), *MotionReport);
    }
    else
    {
        MotionVideoPath.Empty();
        LastActionMessage = Message;
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Motion] %s"), *Message);
    }
    if (!bSettingsRestored)
    {
        LastActionMessage += TEXT(" WARNING: original graphics settings were not restored exactly.");
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Motion] Original graphics settings were not restored exactly."));
    }
    MotionWorld.Reset();
    MotionViewport.Reset();
    MotionController.Reset();
    MotionPawn.Reset();
    MotionHeldDirection = 0;
    bMotionStatUnitWasEnabled = false;
    ArchiveReport(TEXT("Motion:") + MotionDirectory, TEXT("Motion comparison"),
        (bSuccess ? FString(TEXT("completed\n")) : FString(TEXT("stopped\n"))) + LastActionMessage
        + TEXT("\n") + MotionReport);
}

void UGraphicsOptimizerSubsystem::CancelMotionComparison()
{
    if (IsMotionComparisonRunning()) FinishMotionComparison(false, TEXT("Motion comparison cancelled."));
}

bool UGraphicsOptimizerSubsystem::ScanShadowCasters()
{
    ShadowCandidates.Reset();
    ShadowCandidateList.Empty();
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World || !World->IsGameWorld() || Runner.IsRunning() || IsVisualComparisonRunning() ||
        IsMotionComparisonRunning() || IsShadowStudyRunning() || IsGpuTraceCaptureRunning() || IsLumenDetailStudyRunning())
    {
        LastActionMessage = TEXT("Shadow scan needs an idle PIE/game world.");
        return false;
    }
    TMap<UStaticMesh*, int32> GroupIndices;
    int32 EligibleComponents = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor || Actor->IsA<APawn>()) continue;
        TArray<UStaticMeshComponent*> Components;
        Actor->GetComponents<UStaticMeshComponent>(Components);
        for (UStaticMeshComponent* Component : Components)
        {
            if (!Component || !Component->IsRegistered() || !Component->IsVisible() ||
                !Component->CastShadow || !Component->bCastDynamicShadow ||
                !Component->WasRecentlyRendered(1.0f)) continue;
            UStaticMesh* Mesh = Component->GetStaticMesh();
            if (!Mesh) continue;
            ++EligibleComponents;
            int32* GroupIndex = GroupIndices.Find(Mesh);
            if (!GroupIndex)
            {
                FShadowCasterGroup& Group = ShadowCandidates.AddDefaulted_GetRef();
                Group.MeshPath = Mesh->GetPathName();
                const int32 NewIndex = ShadowCandidates.Num() - 1;
                GroupIndices.Add(Mesh, NewIndex);
                GroupIndex = GroupIndices.Find(Mesh);
            }
            FShadowCasterGroup& Group = ShadowCandidates[*GroupIndex];
            Group.Components.Add(Component);
            if (Component->Mobility == EComponentMobility::Static) ++Group.StaticCount;
            else if (Component->Mobility == EComponentMobility::Stationary) ++Group.StationaryCount;
            else ++Group.MovableCount;
        }
    }
    ShadowCandidates.Sort([](const FShadowCasterGroup& A, const FShadowCasterGroup& B)
    {
        return A.Components.Num() == B.Components.Num()
            ? A.MeshPath < B.MeshPath : A.Components.Num() > B.Components.Num();
    });
    if (ShadowCandidates.Num() > 20) ShadowCandidates.SetNum(20);
    ShadowCandidateList = FString::Printf(
        TEXT("Recently rendered dynamic-shadow static-mesh components: %d. Mesh groups found: %d (showing up to 20, sorted by component count, not measured cost).\n"),
        EligibleComponents, GroupIndices.Num());
    for (int32 Index = 0; Index < ShadowCandidates.Num(); ++Index)
    {
        const FShadowCasterGroup& Group = ShadowCandidates[Index];
        ShadowCandidateList += FString::Printf(
            TEXT("#%d | %d components (static %d, stationary %d, movable %d) | %s\n"),
            Index, Group.Components.Num(), Group.StaticCount, Group.StationaryCount,
            Group.MovableCount, *Group.MeshPath);
    }
    if (ShadowCandidates.IsEmpty())
        ShadowCandidateList += TEXT("No candidates. Keep the PIE viewport visible on a scene with shadow-casting static meshes, then scan again.\n");
    LastActionMessage = FString::Printf(TEXT("Shadow scan completed: %d eligible components in %d mesh groups."),
        EligibleComponents, GroupIndices.Num());
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][ShadowStudy] %s\n%s"),
        *LastActionMessage, *ShadowCandidateList);
    return !ShadowCandidates.IsEmpty();
}

bool UGraphicsOptimizerSubsystem::StartShadowStudy(int32 CandidateIndex, float MeasurementSeconds)
{
    if (Runner.IsRunning() || IsVisualComparisonRunning() || IsMotionComparisonRunning() ||
        IsShadowStudyRunning() || IsGpuTraceCaptureRunning() || IsLumenDetailStudyRunning() || !ShadowCandidates.IsValidIndex(CandidateIndex))
    {
        LastActionMessage = TEXT("Scan shadow casters, then choose an available group while other experiments are idle.");
        return false;
    }
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
    FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
    APlayerCameraManager* Camera = World ? UGameplayStatics::GetPlayerCameraManager(World, 0) : nullptr;
    if (!World || !World->IsGameWorld() || !Viewport || !Camera || !FApp::HasFocus())
    {
        LastActionMessage = TEXT("Shadow study needs a focused PIE viewport and a player camera.");
        return false;
    }
    const FIntPoint Size = Viewport->GetSizeXY();
    if (Size.X <= 0 || Size.Y <= 0 || static_cast<int64>(Size.X) * Size.Y > 16000000)
    {
        LastActionMessage = TEXT("PIE viewport size is invalid or exceeds 16 million pixels.");
        return false;
    }
    int32 LiveComponents = 0;
    for (const TWeakObjectPtr<UStaticMeshComponent>& WeakComponent : ShadowCandidates[CandidateIndex].Components)
    {
        const UStaticMeshComponent* Component = WeakComponent.Get();
        if (Component && Component->GetWorld() == World && Component->IsRegistered() && Component->CastShadow)
            ++LiveComponents;
    }
    if (LiveComponents == 0)
    {
        LastActionMessage = TEXT("The scanned group is no longer present in this PIE world. Scan again.");
        return false;
    }
    bShadowPausedWorld = !World->IsPaused();
    if (bShadowPausedWorld && !UGameplayStatics::SetGamePaused(World, true))
    {
        bShadowPausedWorld = false;
        LastActionMessage = TEXT("Could not pause gameplay for the static shadow study.");
        return false;
    }
    ShadowWorld = World;
    ShadowViewport = ViewportClient;
    ShadowOriginalQuality = Scalability::GetQualityLevels();
    ShadowCameraLocation = Camera->GetCameraLocation();
    ShadowCameraRotation = Camera->GetCameraRotation();
    ShadowImageSize = Size;
    ShadowSelectedCandidate = CandidateIndex;
    ShadowMeasurementSeconds = FMath::Clamp(MeasurementSeconds, 1.0f, 120.0f);
    ShadowFrameRateCapFPS = GEngine ? GEngine->GetMaxTickRate(0.0f) : 0.0f;
    ShadowOriginalFlags.Reset();
    ShadowBaselinePixels.Reset();
    ShadowBaselinePerformance = FGraphicsPerformanceSummary();
    ShadowCandidatePerformance = FGraphicsPerformanceSummary();
    ShadowStudyReport.Empty();
    ShadowStudyImagePath.Empty();
    bLastShadowStudyCompleted = false;
    ShadowStudyDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("GraphicsOptimizer"), TEXT("ShadowStudies"),
        FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") +
        FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));
    IFileManager::Get().MakeDirectory(*ShadowStudyDirectory, true);
    ShadowStudyStage = EShadowStudyStage::WarmupBaseline;
    ShadowBenchmark.BeginWarmup(1.0f);
    LastActionMessage = FString::Printf(TEXT("Shadow A/B started for group #%d (%d components). Gameplay paused; original CastShadow flags will be restored."),
        CandidateIndex, LiveComponents);
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][ShadowStudy] %s"), *LastActionMessage);
    return true;
}

bool UGraphicsOptimizerSubsystem::CaptureShadowStudyFrame(bool bBaseline)
{
    UGameViewportClient* ViewportClient = ShadowViewport.Get();
    FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
    if (!Viewport || Viewport->GetSizeXY() != ShadowImageSize) return false;
    TArray<FColor> Pixels;
    if (!Viewport->ReadPixels(Pixels) || Pixels.Num() != static_cast<int64>(ShadowImageSize.X) * ShadowImageSize.Y)
        return false;
    int32 VisiblePixels = 0;
    for (FColor& Pixel : Pixels)
    {
        if (static_cast<int32>(Pixel.R) + Pixel.G + Pixel.B > 24) ++VisiblePixels;
        Pixel.A = 255;
    }
    if (VisiblePixels == 0) return false;
    const FString ImagePath = FPaths::Combine(ShadowStudyDirectory,
        bBaseline ? TEXT("Baseline.png") : TEXT("Candidate.png"));
    if (!SaveComparisonImage(ImagePath, Pixels, ShadowImageSize)) return false;
    if (bBaseline)
    {
        ShadowBaselinePixels = MoveTemp(Pixels);
        return true;
    }
    if (ShadowBaselinePixels.Num() != Pixels.Num()) return false;
    const int32 Width = ShadowImageSize.X;
    const int32 Height = ShadowImageSize.Y;
    TArray<FColor> SideBySide;
    SideBySide.SetNumUninitialized(static_cast<int64>(Width) * Height * 2);
    for (int32 Row = 0; Row < Height; ++Row)
    {
        FMemory::Memcpy(SideBySide.GetData() + static_cast<int64>(Row) * Width * 2,
            ShadowBaselinePixels.GetData() + static_cast<int64>(Row) * Width, Width * sizeof(FColor));
        FMemory::Memcpy(SideBySide.GetData() + static_cast<int64>(Row) * Width * 2 + Width,
            Pixels.GetData() + static_cast<int64>(Row) * Width, Width * sizeof(FColor));
    }
    const FString CombinedPath = FPaths::Combine(ShadowStudyDirectory, TEXT("SideBySide.png"));
    if (!SaveComparisonImage(CombinedPath, SideBySide, FIntPoint(Width * 2, Height))) return false;
    ShadowStudyImagePath = CombinedPath;
    return true;
}

void UGraphicsOptimizerSubsystem::TickShadowStudy()
{
    UWorld* World = ShadowWorld.Get();
    UGameViewportClient* Viewport = ShadowViewport.Get();
    APlayerCameraManager* Camera = World ? UGameplayStatics::GetPlayerCameraManager(World, 0) : nullptr;
    if (!World || !World->IsGameWorld() || !Viewport || !Viewport->Viewport ||
        !Camera || !FApp::HasFocus() || !World->IsPaused() ||
        Viewport->Viewport->GetSizeXY() != ShadowImageSize ||
        Scalability::GetQualityLevels() != ShadowOriginalQuality ||
        FVector::DistSquared(ShadowCameraLocation, Camera->GetCameraLocation()) > 1.0f ||
        !ShadowCameraRotation.Equals(Camera->GetCameraRotation(), 0.1f))
    {
        FString Reasons;
        if (!World || !World->IsGameWorld()) Reasons += TEXT(" game world unavailable;");
        if (!Viewport || !Viewport->Viewport) Reasons += TEXT(" viewport unavailable;");
        else if (Viewport->Viewport->GetSizeXY() != ShadowImageSize)
            Reasons += FString::Printf(TEXT(" viewport size %dx%d -> %dx%d;"),
                ShadowImageSize.X, ShadowImageSize.Y,
                Viewport->Viewport->GetSizeXY().X, Viewport->Viewport->GetSizeXY().Y);
        if (!FApp::HasFocus()) Reasons += TEXT(" application lost focus;");
        if (World && !World->IsPaused()) Reasons += TEXT(" gameplay unpaused;");
        if (Scalability::GetQualityLevels() != ShadowOriginalQuality) Reasons += TEXT(" Scalability changed;");
        if (!Camera) Reasons += TEXT(" camera unavailable;");
        else
        {
            if (FVector::DistSquared(ShadowCameraLocation, Camera->GetCameraLocation()) > 1.0f)
                Reasons += TEXT(" camera position changed;");
            if (!ShadowCameraRotation.Equals(Camera->GetCameraRotation(), 0.1f))
                Reasons += TEXT(" camera rotation changed;");
        }
        FinishShadowStudy(false, TEXT("Shadow study stopped:") + Reasons);
        return;
    }
    if (ShadowStudyStage == EShadowStudyStage::WarmupBaseline && ShadowBenchmark.IsPeriodComplete())
    {
        ShadowBenchmark.BeginMeasurement(ShadowMeasurementSeconds, Viewport);
        ShadowStudyStage = EShadowStudyStage::MeasuringBaseline;
        LastActionMessage = TEXT("Shadow A/B: measuring baseline with original shadows.");
        return;
    }
    if (ShadowStudyStage == EShadowStudyStage::MeasuringBaseline)
    {
        ShadowBenchmark.TickMeasurement();
        if (!ShadowBenchmark.IsPeriodComplete()) return;
        ShadowBaselinePerformance = ShadowBenchmark.FinishMeasurement();
        if (ShadowBaselinePerformance.FrameSamples < 30)
        {
            FinishShadowStudy(false, TEXT("Baseline had fewer than 30 frame samples."));
            return;
        }
        if (!CaptureShadowStudyFrame(true))
        {
            FinishShadowStudy(false, TEXT("Could not save a visible baseline screenshot."));
            return;
        }
        for (const TWeakObjectPtr<UStaticMeshComponent>& WeakComponent : ShadowCandidates[ShadowSelectedCandidate].Components)
        {
            UStaticMeshComponent* Component = WeakComponent.Get();
            if (!Component || Component->GetWorld() != World || !Component->IsRegistered() || !Component->CastShadow)
                continue;
            FShadowOriginalFlag& Original = ShadowOriginalFlags.AddDefaulted_GetRef();
            Original.Component = Component;
            Original.bCastShadow = Component->CastShadow != 0;
            Component->SetCastShadow(false);
        }
        if (ShadowOriginalFlags.IsEmpty())
        {
            FinishShadowStudy(false, TEXT("No eligible shadow flags remained when the candidate experiment began."));
            return;
        }
        ShadowBenchmark.BeginWarmup(1.0f);
        ShadowStudyStage = EShadowStudyStage::WarmupCandidate;
        LastActionMessage = FString::Printf(TEXT("Baseline %.2f FPS; warming up with CastShadow disabled on %d components."),
            ShadowBaselinePerformance.AverageFPS, ShadowOriginalFlags.Num());
        return;
    }
    if (ShadowStudyStage == EShadowStudyStage::WarmupCandidate && ShadowBenchmark.IsPeriodComplete())
    {
        ShadowBenchmark.BeginMeasurement(ShadowMeasurementSeconds, Viewport);
        ShadowStudyStage = EShadowStudyStage::MeasuringCandidate;
        LastActionMessage = TEXT("Shadow A/B: measuring the selected mesh group with CastShadow disabled.");
        return;
    }
    if (ShadowStudyStage == EShadowStudyStage::MeasuringCandidate)
    {
        ShadowBenchmark.TickMeasurement();
        if (!ShadowBenchmark.IsPeriodComplete()) return;
        ShadowCandidatePerformance = ShadowBenchmark.FinishMeasurement();
        if (ShadowCandidatePerformance.FrameSamples < 30)
        {
            FinishShadowStudy(false, TEXT("Shadow candidate had fewer than 30 frame samples."));
            return;
        }
        if (!CaptureShadowStudyFrame(false))
        {
            FinishShadowStudy(false, TEXT("Could not save the candidate screenshot or side-by-side image."));
            return;
        }
        FinishShadowStudy(true, TEXT("Shadow A/B study completed."));
    }
}

void UGraphicsOptimizerSubsystem::FinishShadowStudy(bool bSuccess, const FString& Reason)
{
    const int32 ChangedCount = ShadowOriginalFlags.Num();
    int32 RestoreFailures = 0;
    for (const FShadowOriginalFlag& Entry : ShadowOriginalFlags)
    {
        if (UStaticMeshComponent* Component = Entry.Component.Get())
        {
            Component->SetCastShadow(Entry.bCastShadow);
            if ((Component->CastShadow != 0) != Entry.bCastShadow) ++RestoreFailures;
        }
    }
    ShadowOriginalFlags.Reset();
    if (bShadowPausedWorld)
    {
        if (UWorld* World = ShadowWorld.Get())
            if (!UGameplayStatics::SetGamePaused(World, false)) ++RestoreFailures;
    }
    const bool bComplete = bSuccess && RestoreFailures == 0;
    bLastShadowStudyCompleted = bComplete;
    const FString MeshPath = ShadowCandidates.IsValidIndex(ShadowSelectedCandidate)
        ? ShadowCandidates[ShadowSelectedCandidate].MeshPath : TEXT("unavailable");
    const FString BaselineGPU = ShadowBaselinePerformance.GPUSamples > 0
        ? FString::Printf(TEXT("%.2f ms (%d samples)"), ShadowBaselinePerformance.AverageGPUMS,
            ShadowBaselinePerformance.GPUSamples) : TEXT("unavailable");
    const FString CandidateGPU = ShadowCandidatePerformance.GPUSamples > 0
        ? FString::Printf(TEXT("%.2f ms (%d samples)"), ShadowCandidatePerformance.AverageGPUMS,
            ShadowCandidatePerformance.GPUSamples) : TEXT("unavailable");
    const FString GPUDifference = ShadowBaselinePerformance.GPUSamples > 0 && ShadowCandidatePerformance.GPUSamples > 0
        ? FString::Printf(TEXT("%+.2f ms"), ShadowCandidatePerformance.AverageGPUMS - ShadowBaselinePerformance.AverageGPUMS)
        : TEXT("unavailable");
    ShadowStudyReport = FString::Printf(
        TEXT("Phase 3 shadow caster A/B | %s\nMesh group #%d: %s\nComponents temporarily changed: %d (CastShadow false, then restored)\nThe plugin did not change Scalability. Shadow quality %d, effects quality %d, resolution setting %.0f (0 means project default). Gameplay was paused; camera was checked each frame.\nMeasurement: %.1f sec per side, 1 sec warmup each. Effective max tick rate: %.1f FPS.\nBaseline: %.2f FPS | 1%% low %.2f FPS | frame %.2f ms | render %.2f ms | GPU %s\nCandidate: %.2f FPS | 1%% low %.2f FPS | frame %.2f ms | render %.2f ms | GPU %s\nCandidate - baseline: %+.2f FPS | GPU %s\n"),
        bComplete ? TEXT("completed") : TEXT("stopped"), ShadowSelectedCandidate, *MeshPath, ChangedCount,
        ShadowOriginalQuality.ShadowQuality, ShadowOriginalQuality.EffectsQuality, ShadowOriginalQuality.ResolutionQuality,
        ShadowMeasurementSeconds, ShadowFrameRateCapFPS,
        ShadowBaselinePerformance.AverageFPS, ShadowBaselinePerformance.OnePercentLowFPS,
        ShadowBaselinePerformance.AverageFrameMS, ShadowBaselinePerformance.AverageRenderMS,
        *BaselineGPU,
        ShadowCandidatePerformance.AverageFPS, ShadowCandidatePerformance.OnePercentLowFPS,
        ShadowCandidatePerformance.AverageFrameMS, ShadowCandidatePerformance.AverageRenderMS,
        *CandidateGPU,
        ShadowCandidatePerformance.AverageFPS - ShadowBaselinePerformance.AverageFPS, *GPUDifference);
    if (ShadowFrameRateCapFPS > 0.0f &&
        (ShadowBaselinePerformance.AverageFPS >= ShadowFrameRateCapFPS * 0.98 ||
         ShadowCandidatePerformance.AverageFPS >= ShadowFrameRateCapFPS * 0.98))
        ShadowStudyReport += TEXT("Frame cap may hide the full FPS difference.\n");
    ShadowStudyReport += TEXT("This measures whole-frame effects of one mesh group, not the GPU shadow pass in isolation. A speed gain is not proof that the visual change is acceptable.\n");
    ShadowStudyReport += TEXT("Result: ") + Reason + TEXT("\n");
    if (RestoreFailures > 0)
        ShadowStudyReport += FString::Printf(TEXT("WARNING: %d restoration checks failed.\n"), RestoreFailures);
    ShadowStudyReport += TEXT("A/B image: ") + (ShadowStudyImagePath.IsEmpty() ? TEXT("unavailable") : ShadowStudyImagePath) + TEXT("\n");
    ShadowStudyReport += TEXT("Output directory: ") + ShadowStudyDirectory + TEXT("\n");
    const FString ReportPath = FPaths::Combine(ShadowStudyDirectory, TEXT("ShadowReport.txt"));
    const bool bReportSaved = !ShadowStudyDirectory.IsEmpty() &&
        FFileHelper::SaveStringToFile(ShadowStudyReport, *ReportPath);
    ArchiveReport(TEXT("Shadow:") + ShadowStudyDirectory, TEXT("Shadow caster A/B"), ShadowStudyReport);
    ShadowStudyStage = EShadowStudyStage::None;
    ShadowWorld.Reset();
    ShadowViewport.Reset();
    ShadowBaselinePixels.Reset();
    bShadowPausedWorld = false;
    if (bComplete)
        LastActionMessage = bReportSaved
            ? FString::Printf(TEXT("Shadow A/B complete. Original flags restored. Report: %s"), *ReportPath)
            : TEXT("Shadow A/B complete. Original flags restored. Could not save ShadowReport.txt; use Copy Shadow Report.");
    else
        LastActionMessage = Reason +
            (RestoreFailures > 0 ? TEXT(" WARNING: some shadow flags or pause state could not be restored.") : TEXT(""));
    if (bComplete)
    {
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][ShadowStudy] %s\n%s"),
            *LastActionMessage, *ShadowStudyReport);
    }
    else
    {
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][ShadowStudy] %s\n%s"),
            *LastActionMessage, *ShadowStudyReport);
    }
}

void UGraphicsOptimizerSubsystem::CancelShadowStudy()
{
    if (IsShadowStudyRunning()) FinishShadowStudy(false, TEXT("Shadow A/B study cancelled."));
}

bool UGraphicsOptimizerSubsystem::StartGpuTraceCapture(float MeasurementSeconds, bool bFreezeScene)
{
    LastActionMessage.Empty();
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    UGameViewportClient* ViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
    APlayerCameraManager* Camera = World ? UGameplayStatics::GetPlayerCameraManager(World, 0) : nullptr;
    if (!World || !World->IsGameWorld() || !ViewportClient || !ViewportClient->Viewport ||
        !Camera || !FApp::HasFocus() || Runner.IsRunning() || IsVisualComparisonRunning() ||
        IsMotionComparisonRunning() || IsShadowStudyRunning() || IsGpuTraceCaptureRunning() || IsLumenDetailStudyRunning())
    {
        LastActionMessage = TEXT("GPU capture needs an idle, focused PIE game viewport and a player camera.");
        return false;
    }
    if (FTraceAuxiliary::IsConnected())
    {
        LastActionMessage = TEXT("Another Unreal trace is already recording. Stop that trace before this capture so its channels and file are not changed.");
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][GpuTrace] %s"), *LastActionMessage);
        return false;
    }
    if (FullAnalysisStage == EFullAnalysisStage::Complete || FullAnalysisStage == EFullAnalysisStage::Failed)
    {
        FullAnalysisStage = EFullAnalysisStage::None;
        FullAnalysisReport.Empty();
    }
    bGpuPassStudy = false;
    bGpuPassStudyCompleted = false;
    GpuPassStudyTraces.Reset();
    GpuPassStudyPerformance.Reset();
    GpuPassStudyBaselinePixels.Reset();
    GpuPassStudyGIImagePath.Empty();
    GpuPassStudyShadowImagePath.Empty();
    bGpuTracePausedWorld = bFreezeScene && !World->IsPaused();
    if (bGpuTracePausedWorld && !UGameplayStatics::SetGamePaused(World, true))
    {
        bGpuTracePausedWorld = false;
        LastActionMessage = TEXT("Could not pause gameplay for static GPU capture.");
        return false;
    }
    GpuTraceWorld = World;
    GpuTraceViewport = ViewportClient;
    GpuTraceCameraLocation = Camera->GetCameraLocation();
    GpuTraceCameraRotation = Camera->GetCameraRotation();
    bGpuTraceFreezeScene = bFreezeScene;
    GpuTraceMeasurementSeconds = FMath::Clamp(MeasurementSeconds, 1.0f, 120.0f);
    GpuTraceTickCapFPS = GEngine ? GEngine->GetMaxTickRate(0.0f) : 0.0f;
    GpuTraceQuality = Scalability::GetQualityLevels();
    GpuTracePerformance = FGraphicsPerformanceSummary();
    GpuTraceReport.Empty();
    GpuTraceDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("GraphicsOptimizer"), TEXT("GpuTraces"),
        FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") + FGuid::NewGuid().ToString().Left(8)));
    GpuTracePath = FPaths::Combine(GpuTraceDirectory, TEXT("GpuMeasurement.utrace"));
    if (!IFileManager::Get().MakeDirectory(*GpuTraceDirectory, true))
    {
        if (bGpuTracePausedWorld) UGameplayStatics::SetGamePaused(World, false);
        bGpuTracePausedWorld = false;
        GpuTraceWorld.Reset();
        GpuTraceViewport.Reset();
        LastActionMessage = TEXT("Could not create the GPU trace output directory.");
        return false;
    }
    GpuTraceStage = EGpuTraceStage::Warmup;
    GpuTraceBenchmark.BeginWarmup(1.0f);
    LastActionMessage = TEXT("GPU trace: one-second warmup started. Keep the PIE viewport focused and visible.");
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][GpuTrace] %s Output: %s"), *LastActionMessage, *GpuTracePath);
    return true;
}

bool UGraphicsOptimizerSubsystem::StartGpuPassStudy(float MeasurementSeconds)
{
    if (!StartGpuTraceCapture(MeasurementSeconds, true)) return false;
    GpuPassStudyOriginalQuality = GpuTraceQuality;
    if (GpuPassStudyOriginalQuality.GlobalIlluminationQuality <= 0 ||
        GpuPassStudyOriginalQuality.ShadowQuality <= 0)
    {
        FinishGpuTraceCapture(false, TEXT("GPU pass study needs GI and Shadow quality above 0."));
        return false;
    }
    FViewport* Viewport = GpuTraceViewport.IsValid() ? GpuTraceViewport->Viewport : nullptr;
    GpuPassStudyImageSize = Viewport ? Viewport->GetSizeXY() : FIntPoint::ZeroValue;
    if (GpuPassStudyImageSize.X <= 0 || GpuPassStudyImageSize.Y <= 0 ||
        static_cast<int64>(GpuPassStudyImageSize.X) * GpuPassStudyImageSize.Y > 16000000)
    {
        FinishGpuTraceCapture(false, TEXT("PIE viewport size is invalid or too large for GPU pass A/B images."));
        return false;
    }
    bGpuPassStudy = true;
    GpuPassStudySegment = 0;
    GpuPassStudyExpectedQuality = GpuPassStudyOriginalQuality;
    GpuPassStudyTraces.SetNum(3);
    GpuPassStudyPerformance.SetNum(3);
    GpuTracePath = FPaths::Combine(GpuTraceDirectory, TEXT("Baseline.utrace"));
    LastActionMessage = TEXT("GPU pass A/B: warming up the baseline. Gameplay is paused; camera and settings will be checked.");
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][GpuPassStudy] %s Baseline trace: %s"),
        *LastActionMessage, *GpuTracePath);
    return true;
}

void UGraphicsOptimizerSubsystem::TickGpuTraceCapture()
{
    UWorld* World = GpuTraceWorld.Get();
    UGameViewportClient* ViewportClient = GpuTraceViewport.Get();
    APlayerCameraManager* Camera = World ? UGameplayStatics::GetPlayerCameraManager(World, 0) : nullptr;
    if (!World || !ViewportClient || !ViewportClient->Viewport || !Camera || !FApp::HasFocus())
    {
        FinishGpuTraceCapture(false, TEXT("PIE viewport, camera, or application focus was lost."));
        return;
    }
    if (bGpuPassStudy && (!World->IsPaused() ||
        ViewportClient->Viewport->GetSizeXY() != GpuPassStudyImageSize ||
        Scalability::GetQualityLevels() != GpuPassStudyExpectedQuality))
    {
        FinishGpuTraceCapture(false, TEXT("GPU pass A/B stopped: pause state, viewport size, or graphics settings changed."));
        return;
    }
    if (bGpuTraceFreezeScene &&
        (FVector::DistSquared(GpuTraceCameraLocation, Camera->GetCameraLocation()) > 1.0f ||
         !GpuTraceCameraRotation.Equals(Camera->GetCameraRotation(), 0.1f)))
    {
        FinishGpuTraceCapture(false, TEXT("Camera moved during static GPU capture."));
        return;
    }
    if (GpuTraceStage == EGpuTraceStage::Warmup)
    {
        if (!GpuTraceBenchmark.IsPeriodComplete()) return;
        FTraceAuxiliary::FOptions TraceOptions;
        TraceOptions.bExcludeTail = true;
        TStringBuilder<256> ChannelsBefore;
        FTraceAuxiliary::GetActiveChannelsString(ChannelsBefore);
        const bool bStarted = FTraceAuxiliary::Start(FTraceAuxiliary::EConnectionType::File, *GpuTracePath,
            TEXT("cpu,gpu,frame,bookmark,region"), &TraceOptions);
        TStringBuilder<256> ChannelsAfter;
        FTraceAuxiliary::GetActiveChannelsString(ChannelsAfter);
        TArray<FString> PreviousChannelNames;
        TArray<FString> CurrentChannelNames;
        FString(ChannelsBefore.ToString()).ParseIntoArray(PreviousChannelNames, TEXT(","), true);
        FString(ChannelsAfter.ToString()).ParseIntoArray(CurrentChannelNames, TEXT(","), true);
        TArray<FString> NewChannelNames;
        for (const FString& Name : CurrentChannelNames)
            if (!PreviousChannelNames.Contains(Name)) NewChannelNames.Add(Name);
        GpuTraceNewChannels = FString::Join(NewChannelNames, TEXT(","));
        if (!bStarted)
        {
            if (!GpuTraceNewChannels.IsEmpty()) FTraceAuxiliary::DisableChannels(*GpuTraceNewChannels);
            GpuTraceNewChannels.Empty();
            FinishGpuTraceCapture(false, TEXT("UE trace could not start. Check Output Log and whether another trace is active."));
            return;
        }
        bGpuTraceOwnsTrace = true;
        TRACE_BOOKMARK(TEXT("GraphicsOptimizer GPU measurement START"));
        TRACE_BEGIN_REGION(TEXT("GraphicsOptimizerGpuMeasurement"));
        GpuTraceBenchmark.BeginMeasurement(GpuTraceMeasurementSeconds, ViewportClient);
        GpuTraceStage = EGpuTraceStage::Measuring;
        LastActionMessage = TEXT("GPU trace: measuring and recording GPU timing events.");
        return;
    }
    GpuTraceBenchmark.TickMeasurement();
    if (!GpuTraceBenchmark.IsPeriodComplete()) return;
    GpuTracePerformance = GpuTraceBenchmark.FinishMeasurement();
    if (bGpuPassStudy)
    {
        FinishGpuPassStudySegment();
        return;
    }
    FinishGpuTraceCapture(GpuTracePerformance.FrameSamples >= 30,
        GpuTracePerformance.FrameSamples >= 30 ? TEXT("GPU trace capture completed.") : TEXT("Fewer than 30 frame samples; repeat the capture."));
}

bool UGraphicsOptimizerSubsystem::CaptureGpuPassStudyImage()
{
    UGameViewportClient* ViewportClient = GpuTraceViewport.Get();
    FViewport* Viewport = ViewportClient ? ViewportClient->Viewport : nullptr;
    if (!Viewport || Viewport->GetSizeXY() != GpuPassStudyImageSize) return false;
    TArray<FColor> Pixels;
    if (!Viewport->ReadPixels(Pixels) ||
        Pixels.Num() != static_cast<int64>(GpuPassStudyImageSize.X) * GpuPassStudyImageSize.Y) return false;
    int32 VisiblePixels = 0;
    for (FColor& Pixel : Pixels)
    {
        if (static_cast<int32>(Pixel.R) + Pixel.G + Pixel.B > 24) ++VisiblePixels;
        Pixel.A = 255;
    }
    if (VisiblePixels == 0) return false;
    const TCHAR* Name = GpuPassStudySegment == 0 ? TEXT("Baseline") :
        GpuPassStudySegment == 1 ? TEXT("GI") : TEXT("Shadow");
    if (!SaveComparisonImage(FPaths::Combine(GpuTraceDirectory,
        FString::Printf(TEXT("%s.png"), Name)), Pixels, GpuPassStudyImageSize)) return false;
    if (GpuPassStudySegment == 0)
    {
        GpuPassStudyBaselinePixels = MoveTemp(Pixels);
        return true;
    }
    if (GpuPassStudyBaselinePixels.Num() != Pixels.Num()) return false;
    const int32 Width = GpuPassStudyImageSize.X;
    const int32 Height = GpuPassStudyImageSize.Y;
    TArray<FColor> SideBySide;
    SideBySide.SetNumUninitialized(static_cast<int64>(Width) * Height * 2);
    for (int32 Row = 0; Row < Height; ++Row)
    {
        FMemory::Memcpy(SideBySide.GetData() + static_cast<int64>(Row) * Width * 2,
            GpuPassStudyBaselinePixels.GetData() + static_cast<int64>(Row) * Width, Width * sizeof(FColor));
        FMemory::Memcpy(SideBySide.GetData() + static_cast<int64>(Row) * Width * 2 + Width,
            Pixels.GetData() + static_cast<int64>(Row) * Width, Width * sizeof(FColor));
    }
    FString& ImagePath = GpuPassStudySegment == 1 ? GpuPassStudyGIImagePath : GpuPassStudyShadowImagePath;
    ImagePath = FPaths::Combine(GpuTraceDirectory,
        GpuPassStudySegment == 1 ? TEXT("BaselineVsGI.png") : TEXT("BaselineVsShadow.png"));
    return SaveComparisonImage(ImagePath, SideBySide, FIntPoint(Width * 2, Height));
}

void UGraphicsOptimizerSubsystem::FinishGpuPassStudySegment()
{
    if (GpuTracePerformance.FrameSamples < 30 || GpuTracePerformance.GPUSamples < 30)
    {
        FinishGpuTraceCapture(false, TEXT("GPU pass A/B had fewer than 30 frame or GPU samples."));
        return;
    }
    if (bGpuTraceOwnsTrace)
    {
        TRACE_END_REGION(TEXT("GraphicsOptimizerGpuMeasurement"));
        TRACE_BOOKMARK(TEXT("GraphicsOptimizer GPU measurement END"));
        const bool bStopped = FTraceAuxiliary::Stop();
        if (!GpuTraceNewChannels.IsEmpty()) FTraceAuxiliary::DisableChannels(*GpuTraceNewChannels);
        GpuTraceNewChannels.Empty();
        bGpuTraceOwnsTrace = false;
        if (!bStopped)
        {
            FinishGpuTraceCapture(false, TEXT("Could not stop a GPU pass study trace cleanly."));
            return;
        }
    }
    if (IFileManager::Get().FileSize(*GpuTracePath) <= 0 || !CaptureGpuPassStudyImage())
    {
        FinishGpuTraceCapture(false, TEXT("Could not save the GPU pass trace or a visible comparison image."));
        return;
    }
    GpuPassStudyTraces[GpuPassStudySegment] = GpuTracePath;
    GpuPassStudyPerformance[GpuPassStudySegment] = GpuTracePerformance;
    ++GpuPassStudySegment;
    if (GpuPassStudySegment == 3)
    {
        FinishGpuTraceCapture(true, TEXT("GPU pass A/B completed."));
        return;
    }
    GpuPassStudyExpectedQuality = GpuPassStudyOriginalQuality;
    if (GpuPassStudySegment == 1)
        --GpuPassStudyExpectedQuality.GlobalIlluminationQuality;
    else
        --GpuPassStudyExpectedQuality.ShadowQuality;
    Scalability::SetQualityLevels(GpuPassStudyExpectedQuality);
    if (Scalability::GetQualityLevels() != GpuPassStudyExpectedQuality)
    {
        FinishGpuTraceCapture(false, TEXT("Could not apply the next GPU pass study quality setting exactly."));
        return;
    }
    GpuTracePath = FPaths::Combine(GpuTraceDirectory,
        GpuPassStudySegment == 1 ? TEXT("GI.utrace") : TEXT("Shadow.utrace"));
    GpuTraceBenchmark.BeginWarmup(1.0f);
    GpuTraceStage = EGpuTraceStage::Warmup;
    LastActionMessage = GpuPassStudySegment == 1
        ? TEXT("GPU pass A/B: baseline saved; warming up GI one level lower.")
        : TEXT("GPU pass A/B: GI saved; warming up Shadow one level lower with GI restored.");
}

void UGraphicsOptimizerSubsystem::FinishGpuTraceCapture(bool bSuccess, const FString& Reason)
{
    if (GpuTraceStage == EGpuTraceStage::None) return;
    if (bGpuTraceOwnsTrace)
    {
        TRACE_END_REGION(TEXT("GraphicsOptimizerGpuMeasurement"));
        TRACE_BOOKMARK(TEXT("GraphicsOptimizer GPU measurement END"));
        if (!FTraceAuxiliary::Stop()) bSuccess = false;
        if (!GpuTraceNewChannels.IsEmpty()) FTraceAuxiliary::DisableChannels(*GpuTraceNewChannels);
        GpuTraceNewChannels.Empty();
        bGpuTraceOwnsTrace = false;
    }
    if (bGpuPassStudy)
    {
        Scalability::SetQualityLevels(GpuPassStudyOriginalQuality);
        const bool bRestored = Scalability::GetQualityLevels() == GpuPassStudyOriginalQuality;
        bSuccess = bSuccess && bRestored && GpuPassStudyTraces.Num() == 3 &&
            GpuPassStudyTraces[0].Len() > 0 && GpuPassStudyTraces[1].Len() > 0 && GpuPassStudyTraces[2].Len() > 0;
        if (bGpuTracePausedWorld)
        {
            if (UWorld* World = GpuTraceWorld.Get())
                if (!UGameplayStatics::SetGamePaused(World, false)) bSuccess = false;
        }
        bGpuPassStudyCompleted = bSuccess;
        GpuTraceReport = FString::Printf(
            TEXT("Phase 3 GPU pass A/B | %s\nResult: %s\nFixed camera: %s / %s\nMeasurement: %.1f sec per setting after 1 sec warmup; gameplay paused. Effective max tick rate %.1f FPS.\nOriginal quality: resolution %.1f, shadow %d, GI %d, reflection %d, post %d, texture %d, effects %d\nBaseline: %.2f FPS | 1%% low %.2f | GPU %.2f ms\nGI %d: %.2f FPS (%+.2f) | 1%% low %.2f | GPU %.2f ms (%+.2f)\nShadow %d: %.2f FPS (%+.2f) | 1%% low %.2f | GPU %.2f ms (%+.2f)\nBaseline trace: %s\nGI trace: %s\nShadow trace: %s\nBaseline vs GI image: %s\nBaseline vs Shadow image: %s\nOutput directory: %s\nOriginal Scalability %s. A whole-frame gain does not prove visual acceptability; compare images and GPU pass timers.\n"),
            bSuccess ? TEXT("completed") : TEXT("stopped"), *Reason,
            *GpuTraceCameraLocation.ToCompactString(), *GpuTraceCameraRotation.ToCompactString(),
            GpuTraceMeasurementSeconds, GpuTraceTickCapFPS,
            GpuPassStudyOriginalQuality.ResolutionQuality, GpuPassStudyOriginalQuality.ShadowQuality,
            GpuPassStudyOriginalQuality.GlobalIlluminationQuality, GpuPassStudyOriginalQuality.ReflectionQuality,
            GpuPassStudyOriginalQuality.PostProcessQuality, GpuPassStudyOriginalQuality.TextureQuality,
            GpuPassStudyOriginalQuality.EffectsQuality,
            GpuPassStudyPerformance.IsValidIndex(0) ? GpuPassStudyPerformance[0].AverageFPS : 0.0,
            GpuPassStudyPerformance.IsValidIndex(0) ? GpuPassStudyPerformance[0].OnePercentLowFPS : 0.0,
            GpuPassStudyPerformance.IsValidIndex(0) ? GpuPassStudyPerformance[0].AverageGPUMS : 0.0,
            GpuPassStudyOriginalQuality.GlobalIlluminationQuality - 1,
            GpuPassStudyPerformance.IsValidIndex(1) ? GpuPassStudyPerformance[1].AverageFPS : 0.0,
            GpuPassStudyPerformance.Num() == 3 ? GpuPassStudyPerformance[1].AverageFPS - GpuPassStudyPerformance[0].AverageFPS : 0.0,
            GpuPassStudyPerformance.IsValidIndex(1) ? GpuPassStudyPerformance[1].OnePercentLowFPS : 0.0,
            GpuPassStudyPerformance.IsValidIndex(1) ? GpuPassStudyPerformance[1].AverageGPUMS : 0.0,
            GpuPassStudyPerformance.Num() == 3 ? GpuPassStudyPerformance[1].AverageGPUMS - GpuPassStudyPerformance[0].AverageGPUMS : 0.0,
            GpuPassStudyOriginalQuality.ShadowQuality - 1,
            GpuPassStudyPerformance.IsValidIndex(2) ? GpuPassStudyPerformance[2].AverageFPS : 0.0,
            GpuPassStudyPerformance.Num() == 3 ? GpuPassStudyPerformance[2].AverageFPS - GpuPassStudyPerformance[0].AverageFPS : 0.0,
            GpuPassStudyPerformance.IsValidIndex(2) ? GpuPassStudyPerformance[2].OnePercentLowFPS : 0.0,
            GpuPassStudyPerformance.IsValidIndex(2) ? GpuPassStudyPerformance[2].AverageGPUMS : 0.0,
            GpuPassStudyPerformance.Num() == 3 ? GpuPassStudyPerformance[2].AverageGPUMS - GpuPassStudyPerformance[0].AverageGPUMS : 0.0,
            GpuPassStudyTraces.IsValidIndex(0) ? *GpuPassStudyTraces[0] : TEXT("unavailable"),
            GpuPassStudyTraces.IsValidIndex(1) ? *GpuPassStudyTraces[1] : TEXT("unavailable"),
            GpuPassStudyTraces.IsValidIndex(2) ? *GpuPassStudyTraces[2] : TEXT("unavailable"),
            *GpuPassStudyGIImagePath, *GpuPassStudyShadowImagePath, *GpuTraceDirectory,
            bRestored ? TEXT("restored") : TEXT("RESTORE FAILED"));
        if (GpuTraceTickCapFPS > 0.0f)
        {
            for (const FGraphicsPerformanceSummary& Summary : GpuPassStudyPerformance)
            {
                if (Summary.FrameSamples > 0 && Summary.AverageFPS >= GpuTraceTickCapFPS * 0.98f)
                {
                    GpuTraceReport += TEXT("At least one arm is near the engine frame cap; FPS gains may be hidden. Compare GPU times too.\n");
                    break;
                }
            }
        }
        const FString ReportPath = FPaths::Combine(GpuTraceDirectory, TEXT("GpuPassStudyReport.txt"));
        FFileHelper::SaveStringToFile(GpuTraceReport, *ReportPath);
        ArchiveReport(TEXT("GPU:") + GpuTraceDirectory, TEXT("GPU pass A/B"), GpuTraceReport);
        LastActionMessage = Reason + (bRestored ? TEXT(" Original settings restored.") : TEXT(" WARNING: Original settings could not be restored."));
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][GpuPassStudy] %s\n%s"), *LastActionMessage, *GpuTraceReport);
        GpuTraceStage = EGpuTraceStage::None;
        GpuTraceWorld.Reset();
        GpuTraceViewport.Reset();
        GpuPassStudyBaselinePixels.Reset();
        bGpuTracePausedWorld = false;
        return;
    }
    if (bGpuTracePausedWorld)
    {
        if (UWorld* World = GpuTraceWorld.Get())
            if (!UGameplayStatics::SetGamePaused(World, false)) bSuccess = false;
    }
    const int64 TraceBytes = IFileManager::Get().FileSize(*GpuTracePath);
    if (TraceBytes <= 0) bSuccess = false;
    const FString Outcome = bSuccess ? Reason : Reason +
        (TraceBytes <= 0 ? TEXT(" Trace file is empty or missing.") : TEXT(" Capture or pause restoration did not finish cleanly."));
    GpuTraceReport = FString::Printf(
        TEXT("Phase 3 GPU workload trace | %s\nResult: %s\nScene: %s; camera start %s / %s\nSettings: resolution %.1f (0 means project default), shadow %d, GI %d, reflection %d, post %d, texture %d, effects %d\nMeasurement: %.1f sec after 1 sec warmup | effective max tick rate %.1f FPS\nPerformance: %.2f FPS | 1%% low %.2f FPS | frame %.2f ms | game %.2f ms | render %.2f ms | GPU %.2f ms (%d GPU samples, %d frame samples)\nGPU trace: %s (%lld bytes)\nOutput directory: %s\nOpen the .utrace in Unreal Insights > Timing Insights; inspect the GPU track within GraphicsOptimizerGpuMeasurement. Pass timers are inclusive and nested; they must not be added together. The trace records the current graphics settings without changing them.\n"),
        bSuccess ? TEXT("completed") : TEXT("stopped"), *Outcome,
        bGpuTraceFreezeScene ? TEXT("static gameplay pause") : TEXT("live gameplay"),
        *GpuTraceCameraLocation.ToCompactString(), *GpuTraceCameraRotation.ToCompactString(),
        GpuTraceQuality.ResolutionQuality, GpuTraceQuality.ShadowQuality,
        GpuTraceQuality.GlobalIlluminationQuality, GpuTraceQuality.ReflectionQuality,
        GpuTraceQuality.PostProcessQuality, GpuTraceQuality.TextureQuality, GpuTraceQuality.EffectsQuality,
        GpuTraceMeasurementSeconds, GpuTraceTickCapFPS,
        GpuTracePerformance.AverageFPS, GpuTracePerformance.OnePercentLowFPS,
        GpuTracePerformance.AverageFrameMS, GpuTracePerformance.AverageGameMS,
        GpuTracePerformance.AverageRenderMS, GpuTracePerformance.AverageGPUMS,
        GpuTracePerformance.GPUSamples, GpuTracePerformance.FrameSamples,
        *GpuTracePath, TraceBytes, *GpuTraceDirectory);
    const FString ReportPath = FPaths::Combine(GpuTraceDirectory, TEXT("GpuTraceReport.txt"));
    const bool bSaved = FFileHelper::SaveStringToFile(GpuTraceReport, *ReportPath);
    ArchiveReport(TEXT("GPU:") + GpuTraceDirectory, TEXT("GPU workload trace"), GpuTraceReport);
    LastActionMessage = FString::Printf(TEXT("%s %s"), *Outcome,
        bSaved ? *FString::Printf(TEXT("Report: %s"), *ReportPath) : TEXT("Could not save GpuTraceReport.txt."));
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][GpuTrace] %s\n%s"), *LastActionMessage, *GpuTraceReport);
    GpuTraceStage = EGpuTraceStage::None;
    GpuTraceWorld.Reset();
    GpuTraceViewport.Reset();
    bGpuTracePausedWorld = false;
}

void UGraphicsOptimizerSubsystem::CancelGpuTraceCapture()
{
    if (IsGpuTraceCaptureRunning()) FinishGpuTraceCapture(false, TEXT("GPU trace capture cancelled."));
}

bool UGraphicsOptimizerSubsystem::SaveCurrentSettingsAsPreset()
{
    if (Runner.IsRunning() || IsVisualComparisonRunning() || IsMotionComparisonRunning() || IsShadowStudyRunning() || IsGpuTraceCaptureRunning() || IsLumenDetailStudyRunning())
    {
        LastActionMessage = TEXT("Wait for the benchmark or comparison capture to finish before saving current settings.");
        return false;
    }
    if (!FGraphicsPresetManager::Save(Scalability::GetQualityLevels()))
    {
        LastActionMessage = TEXT("Could not write the current settings preset.");
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Preset] %s"), *LastActionMessage);
        return false;
    }
    LastActionMessage = FString::Printf(TEXT("Saved current Scalability settings to %s."), *FGraphicsPresetManager::GetPath());
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Preset] %s"), *LastActionMessage);
    return true;
}

bool UGraphicsOptimizerSubsystem::ApplySavedPreset()
{
    Scalability::FQualityLevels Configuration;
    if (!FGraphicsPresetManager::Load(Configuration))
    {
        LastActionMessage = FString::Printf(TEXT("No valid preset found at %s."), *FGraphicsPresetManager::GetPath());
        UE_LOG(LogGraphicsOptimizer, Warning, TEXT("[GraphicsOptimizer][Preset] %s"), *LastActionMessage);
        return false;
    }
    return ApplyConfiguration(Configuration, TEXT("saved preset"));
}

bool UGraphicsOptimizerSubsystem::RestoreAppliedConfiguration()
{
    if (!bHasAppliedConfiguration || Runner.IsRunning() || IsVisualComparisonRunning() || IsMotionComparisonRunning() || IsShadowStudyRunning() || IsGpuTraceCaptureRunning() || IsLumenDetailStudyRunning())
    {
        LastActionMessage = TEXT("No applied configuration can be restored right now.");
        return false;
    }
    Scalability::SetQualityLevels(OriginalBeforeApply);
    if (Scalability::GetQualityLevels() != OriginalBeforeApply)
    {
        LastActionMessage = TEXT("Failed to restore the pre-apply graphics settings.");
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Apply] %s"), *LastActionMessage);
        return false;
    }
    bHasAppliedConfiguration = false;
    LastActionMessage = TEXT("Pre-apply graphics settings restored.");
    if (IsFullAnalysisComplete())
    {
        FullAnalysisDecision = TEXT("Restored pre-apply graphics settings in PIE. No recommendation was saved.");
        WriteFullAnalysisReport();
    }
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Apply] %s"), *LastActionMessage);
    return true;
}

FString UGraphicsOptimizerSubsystem::GetSavedPresetPath() const
{
    return FGraphicsPresetManager::GetPath();
}

void UGraphicsOptimizerSubsystem::CancelBenchmark()
{
    const bool bWasFullAnalysis = IsFullAnalysisRunning();
    if (bWasFullAnalysis) FullAnalysisStage = EFullAnalysisStage::Failed;
    if (IsLumenDetailStudyRunning()) CancelLumenDetailStudy();
    if (IsGpuTraceCaptureRunning()) CancelGpuTraceCapture();
    if (IsShadowStudyRunning()) CancelShadowStudy();
    if (IsMotionComparisonRunning()) CancelMotionComparison();
    if (IsVisualComparisonRunning()) CancelVisualComparison();
    Runner.Cancel();
    ArchiveBenchmarkIfFinished();
    RestoreStatUnit();
    RestoreGameplayPause();
    if (bWasFullAnalysis) FinishFullAnalysis(false, TEXT("Full analysis cancelled by user."));
}

void UGraphicsOptimizerSubsystem::WriteFullAnalysisReport()
{
    const TArray<FGraphicsExperimentResult>& Results = Runner.GetResults();
    const int32 BestIndex = Runner.GetBestResultIndex();
    const FGraphicsExperimentResult* Baseline = Results.IsValidIndex(0) ? &Results[0] : nullptr;
    const FGraphicsExperimentResult* Best = Results.IsValidIndex(BestIndex) ? &Results[BestIndex] : nullptr;
    const FGraphicsValidationResult* Validation = Runner.GetSelectedValidation();
    FullAnalysisReport = FString::Printf(
        TEXT("Graphics Optimizer | full analysis\nState: %s\nOutcome: %s\nTarget: %.1f FPS\nDecision: %s\n\n1. Measured setting search\nBaseline: %s\nMeasured recommendation: %s\nRecommendation changes: %s\nTarget verdict: %s\nOriginal settings were restored after the measurements. The recommendation is not applied automatically.\n"),
        FullAnalysisStage == EFullAnalysisStage::Complete ? TEXT("completed") : TEXT("stopped"),
        *FullAnalysisOutcome, Runner.GetOptions().TargetFPS, *FullAnalysisDecision,
        Baseline ? *FString::Printf(TEXT("%.2f FPS | GPU %.2f ms | 1%% low %.2f FPS"),
            Baseline->Performance.AverageFPS, Baseline->Performance.AverageGPUMS, Baseline->Performance.OnePercentLowFPS) : TEXT("unavailable"),
        Best ? *FString::Printf(TEXT("#%d %s | %.2f FPS | GPU %.2f ms | 1%% low %.2f FPS"),
            BestIndex, *Best->Label, Best->Performance.AverageFPS, Best->Performance.AverageGPUMS,
            Best->Performance.OnePercentLowFPS) : TEXT("unavailable"),
        Best ? *Best->Label : TEXT("unavailable"),
        Best ? (Runner.HasMetOptimizationTarget() ? TEXT("repeatably met") :
            Validation ? TEXT("not repeatably met") :
            !Best->bTargetEvaluationValid ? TEXT("inconclusive (frame cap)") : TEXT("target missed")) : TEXT("unavailable"));
    if (Best && Baseline)
        FullAnalysisReport += FString::Printf(TEXT("Measured difference: %+.2f FPS | GPU %+.2f ms | 1%% low %+.2f FPS\n"),
            Best->Performance.AverageFPS - Baseline->Performance.AverageFPS,
            Best->Performance.AverageGPUMS - Baseline->Performance.AverageGPUMS,
            Best->Performance.OnePercentLowFPS - Baseline->Performance.OnePercentLowFPS);
    if (Validation)
        FullAnalysisReport += FString::Printf(TEXT("Selected validation: median %.2f FPS | range %.2f-%.2f FPS | target met %d/%d | %s\n"),
            Validation->MedianFPS, Validation->MinimumFPS, Validation->MaximumFPS,
            Validation->TargetMetRuns, FOptimizationManager::ValidationRunsPerCandidate,
            Validation->bAllTargetEvaluationsValid ? TEXT("valid") : TEXT("frame-cap verdict inconclusive"));
    FullAnalysisReport += TEXT("\n2. GPU pass A/B\n") +
        (GpuTraceReport.IsEmpty() ? TEXT("not completed") : GpuTraceReport) + TEXT("\n");
    FullAnalysisReport += TEXT("\n3. Local shadow-caster experiments (component-count shortlist, exploratory; not part of automatic setting recommendation)\n");
    if (FullAnalysisShadowObservations.IsEmpty())
        FullAnalysisReport += TEXT("No eligible shadow-caster group was measured.\n");
    else
    {
        int32 BestShadowIndex = INDEX_NONE;
        for (int32 Index = 0; Index < FullAnalysisShadowObservations.Num(); ++Index)
        {
            const FFullShadowObservation& Observation = FullAnalysisShadowObservations[Index];
            FullAnalysisReport += FString::Printf(TEXT("Group %d: %s | FPS %+.2f | GPU saved %.2f ms | image %s\n"),
                Index, *Observation.MeshPath, Observation.FPSGain, Observation.GPUMSGain, *Observation.ImagePath);
            if (BestShadowIndex == INDEX_NONE || Observation.GPUMSGain > FullAnalysisShadowObservations[BestShadowIndex].GPUMSGain)
                BestShadowIndex = Index;
        }
        if (BestShadowIndex != INDEX_NONE)
            FullAnalysisReport += FString::Printf(TEXT("Largest measured shadow-caster GPU saving in this shortlist: group %d (%.2f ms). Review its image; it is not applied by the recommendation button.\n"),
                BestShadowIndex, FullAnalysisShadowObservations[BestShadowIndex].GPUMSGain);
    }
    if (FullAnalysisStage == EFullAnalysisStage::Failed && !bLastShadowStudyCompleted && !ShadowStudyReport.IsEmpty())
        FullAnalysisReport += TEXT("\nInterrupted shadow experiment:\n") + ShadowStudyReport;
    FullAnalysisReport += TEXT("\n4. Engine scene diagnosis (VSM risk and repeated-mesh candidates)\n") +
        (EngineSceneReport.IsEmpty() ? TEXT("not completed\n") : EngineSceneReport);
    FullAnalysisReport += TEXT("\n5. Lumen detail A/B\n") +
        (LumenStudyReport.IsEmpty() ? TEXT("not completed\n") : LumenStudyReport);
    FullAnalysisReport += TEXT("\n6. Visual comparison\nBaseline vs measured recommendation: ") +
        (ComparisonImagePath.IsEmpty() ? TEXT("unavailable") : ComparisonImagePath) + TEXT("\n");
    FullAnalysisReport += TEXT("A measured speed gain does not establish acceptable image quality. Review the side-by-side images before applying or saving any change.\n");
    FullAnalysisReport += TEXT("\n7. All measured setting configurations and repeat validation\n") +
        (LatestBenchmarkReport.IsEmpty() ? TEXT("not completed\n") : LatestBenchmarkReport);
    FullAnalysisReport += TEXT("Output directory: ") + FullAnalysisDirectory + TEXT("\n");
    if (!FullAnalysisDirectory.IsEmpty())
        FFileHelper::SaveStringToFile(FullAnalysisReport,
            *FPaths::Combine(FullAnalysisDirectory, TEXT("FullAnalysisReport.txt")));
    ArchiveReport(TEXT("Full:") + FullAnalysisDirectory, TEXT("Full analysis"), FullAnalysisReport);
}

void UGraphicsOptimizerSubsystem::FinishFullAnalysis(bool bComplete, const FString& Reason)
{
    FullAnalysisOutcome = Reason;
    FullAnalysisStage = bComplete ? EFullAnalysisStage::Complete : EFullAnalysisStage::Failed;
    Scalability::SetQualityLevels(FullAnalysisStartingQuality);
    if (Scalability::GetQualityLevels() != FullAnalysisStartingQuality)
    {
        FullAnalysisStage = EFullAnalysisStage::Failed;
        FullAnalysisDecision = TEXT("Original graphics settings could not be restored; inspect PIE before applying settings.");
    }
    if (bFullAnalysisPausedWorld)
    {
        if (UWorld* World = FullAnalysisWorld.Get())
        {
            if (!UGameplayStatics::SetGamePaused(World, false))
            {
                FullAnalysisStage = EFullAnalysisStage::Failed;
                FullAnalysisDecision = TEXT("Pause-state restoration failed; inspect PIE before applying settings.");
            }
        }
    }
    bFullAnalysisPausedWorld = false;
    FullAnalysisWorld.Reset();
    if (FullAnalysisStage == EFullAnalysisStage::Failed && FullAnalysisDecision.StartsWith(TEXT("Pending:")))
        FullAnalysisDecision = TEXT("No automatic application. Review the partial results and original settings.");
    WriteFullAnalysisReport();
    LastActionMessage = Reason + FString::Printf(TEXT(" Report: %s"),
        *FPaths::Combine(FullAnalysisDirectory, TEXT("FullAnalysisReport.txt")));
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][FullAnalysis] %s\n%s"),
        *LastActionMessage, *FullAnalysisReport);
}

void UGraphicsOptimizerSubsystem::AdvanceFullAnalysis()
{
    switch (FullAnalysisStage)
    {
    case EFullAnalysisStage::Benchmark:
        ArchiveBenchmarkIfFinished();
        if (Runner.GetState() != EGraphicsOptimizerState::Completed ||
            !Runner.GetResults().IsValidIndex(Runner.GetBestResultIndex()))
        {
            FinishFullAnalysis(false, TEXT("Setting search did not produce a valid recommendation."));
            return;
        }
        if (!StartGpuPassStudy(FullAnalysisMeasurementSeconds))
        {
            FinishFullAnalysis(false, TEXT("Could not start the GPU pass A/B stage."));
            return;
        }
        FullAnalysisStage = EFullAnalysisStage::GpuPasses;
        return;
    case EFullAnalysisStage::GpuPasses:
        if (!HasGpuPassStudy())
        {
            FinishFullAnalysis(false, TEXT("GPU pass A/B did not complete."));
            return;
        }
        ScanEngineScene();
        if (ScanShadowCasters())
        {
            FullAnalysisShadowIndex = 0;
            FullAnalysisShadowLimit = FMath::Min(3, ShadowCandidates.Num());
            if (StartShadowStudy(0, FullAnalysisMeasurementSeconds))
            {
                FullAnalysisStage = EFullAnalysisStage::Shadow;
                return;
            }
            FinishFullAnalysis(false, TEXT("Could not start the first shadow-caster experiment."));
            return;
        }
        // The scene may contain no eligible static-mesh shadow casters.
        if (StartLumenDetailStudy(FullAnalysisMeasurementSeconds))
        {
            FullAnalysisStage = EFullAnalysisStage::Lumen;
            return;
        }
        FinishFullAnalysis(false, TEXT("Could not start the Lumen detail A/B stage."));
        return;
    case EFullAnalysisStage::Shadow:
        if (!bLastShadowStudyCompleted)
        {
            FinishFullAnalysis(false, TEXT("A shadow-caster experiment stopped before completion."));
            return;
        }
        {
            FFullShadowObservation& Observation = FullAnalysisShadowObservations.AddDefaulted_GetRef();
            Observation.MeshPath = ShadowCandidates.IsValidIndex(FullAnalysisShadowIndex)
                ? ShadowCandidates[FullAnalysisShadowIndex].MeshPath : TEXT("unavailable");
            Observation.ImagePath = ShadowStudyImagePath;
            Observation.Report = ShadowStudyReport;
            Observation.FPSGain = ShadowCandidatePerformance.AverageFPS - ShadowBaselinePerformance.AverageFPS;
            Observation.GPUMSGain = ShadowBaselinePerformance.AverageGPUMS - ShadowCandidatePerformance.AverageGPUMS;
        }
        ++FullAnalysisShadowIndex;
        if (FullAnalysisShadowIndex < FullAnalysisShadowLimit)
        {
            if (StartShadowStudy(FullAnalysisShadowIndex, FullAnalysisMeasurementSeconds)) return;
            FinishFullAnalysis(false, TEXT("Could not start the next shadow-caster experiment."));
            return;
        }
        if (StartLumenDetailStudy(FullAnalysisMeasurementSeconds))
        {
            FullAnalysisStage = EFullAnalysisStage::Lumen;
            return;
        }
        FinishFullAnalysis(false, TEXT("Could not start the Lumen detail A/B stage."));
        return;
    case EFullAnalysisStage::Lumen:
        if (!bLumenStudyCompleted)
        {
            FinishFullAnalysis(false, TEXT("Lumen detail A/B did not complete."));
            return;
        }
        if (StartVisualComparison(Runner.GetBestResultIndex()))
        {
            FullAnalysisStage = EFullAnalysisStage::Visual;
            return;
        }
        FinishFullAnalysis(false, TEXT("Could not start the final visual comparison."));
        return;
    case EFullAnalysisStage::Visual:
        FinishFullAnalysis(!ComparisonImagePath.IsEmpty(), ComparisonImagePath.IsEmpty()
            ? TEXT("Final image comparison did not complete.") : TEXT("Full analysis completed; choose whether to apply the measured recommendation."));
        return;
    default: return;
    }
}

bool UGraphicsOptimizerSubsystem::ApplyFullAnalysisRecommendation()
{
    if (!IsFullAnalysisComplete()) return false;
    const int32 Index = Runner.GetBestResultIndex();
    if (!ApplyResult(Index)) return false;
    FullAnalysisDecision = FString::Printf(TEXT("Applied measured recommendation #%d to this PIE session; no preset was saved."), Index);
    WriteFullAnalysisReport();
    return true;
}

bool UGraphicsOptimizerSubsystem::KeepOriginalAfterFullAnalysis()
{
    if (!IsFullAnalysisComplete()) return false;
    Scalability::SetQualityLevels(FullAnalysisStartingQuality);
    if (Scalability::GetQualityLevels() != FullAnalysisStartingQuality)
    {
        LastActionMessage = TEXT("Could not restore the settings that were active before full analysis.");
        return false;
    }
    bHasAppliedConfiguration = bFullAnalysisHadAppliedConfiguration;
    OriginalBeforeApply = FullAnalysisPreviousApplyOriginal;
    FullAnalysisDecision = TEXT("Kept the pre-analysis settings in PIE. No recommendation was saved.");
    WriteFullAnalysisReport();
    LastActionMessage = FullAnalysisDecision;
    return true;
}

void UGraphicsOptimizerSubsystem::RestoreStatUnit()
{
    if (bRestoreStatUnit)
    {
        if (UGameViewportClient* Viewport = StatViewport.Get())
        {
            if (Viewport->IsStatEnabled(TEXT("Unit")))
            {
                GEngine->SetEngineStat(Viewport->GetWorld(), Viewport, TEXT("Unit"), false);
                UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][StatUnit] Restored original overlay state."));
            }
        }
    }
    bRestoreStatUnit = false;
    StatViewport.Reset();
}

void UGraphicsOptimizerSubsystem::RestoreGameplayPause()
{
    if (!bRestoreGameplayPause) return;
    if (UWorld* World = PausedWorld.Get())
    {
        if (World->IsPaused() != bPreviousPauseState)
        {
            if (!UGameplayStatics::SetGamePaused(World, bPreviousPauseState))
            {
                UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Benchmark] Failed to restore original gameplay pause state."));
            }
            else
            {
                UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Benchmark] Original gameplay pause state restored."));
            }
        }
    }
    bRestoreGameplayPause = false;
    bHasFixedCamera = false;
    PausedWorld.Reset();
}

void UGraphicsOptimizerSubsystem::OnEndFrame()
{
    ArchiveBenchmarkIfFinished();
    EnsureResolutionProbe();
    if (IsLumenDetailStudyRunning())
    {
        TickLumenDetailStudy();
        if (!IsLumenDetailStudyRunning() && IsFullAnalysisRunning()) AdvanceFullAnalysis();
        return;
    }
    if (IsGpuTraceCaptureRunning())
    {
        TickGpuTraceCapture();
        if (!IsGpuTraceCaptureRunning() && IsFullAnalysisRunning()) AdvanceFullAnalysis();
        return;
    }
    if (IsShadowStudyRunning())
    {
        TickShadowStudy();
        if (!IsShadowStudyRunning() && IsFullAnalysisRunning()) AdvanceFullAnalysis();
        return;
    }
    if (IsMotionComparisonRunning()) { TickMotionComparison(); return; }
    if (IsVisualComparisonRunning())
    {
        TickVisualComparison();
        if (!IsVisualComparisonRunning() && IsFullAnalysisRunning()) AdvanceFullAnalysis();
        return;
    }
    if (!Runner.IsRunning())
    {
        RestoreStatUnit();
        RestoreGameplayPause();
        if (IsFullAnalysisRunning()) AdvanceFullAnalysis();
        return;
    }
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!World || !World->IsGameWorld())
    {
        Runner.Abort(TEXT("Game world became unavailable during benchmark."));
        RestoreStatUnit();
        RestoreGameplayPause();
        if (IsFullAnalysisRunning()) AdvanceFullAnalysis();
        return;
    }
    if (!FApp::HasFocus())
    {
        Runner.Abort(TEXT("Application lost focus during benchmark."));
        RestoreStatUnit();
        RestoreGameplayPause();
        if (IsFullAnalysisRunning()) AdvanceFullAnalysis();
        return;
    }
    if (bHasFixedCamera)
    {
        if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(World, 0))
        {
            if (FVector::DistSquared(FixedCameraLocation, Camera->GetCameraLocation()) > 1.0f ||
                !FixedCameraRotation.Equals(Camera->GetCameraRotation(), 0.1f))
            {
                Runner.Abort(TEXT("Camera moved during static-scene benchmark."));
                RestoreStatUnit();
                RestoreGameplayPause();
                if (IsFullAnalysisRunning()) AdvanceFullAnalysis();
                return;
            }
        }
    }
    float ObservedPrimaryResolutionScale = 0.0f;
    if (ResolutionProbe.IsValid()) ResolutionProbe->GetStablePrimaryPercentage(ObservedPrimaryResolutionScale);
    Runner.Tick(ObservedPrimaryResolutionScale);
    if (!Runner.IsRunning())
    {
        RestoreStatUnit();
        RestoreGameplayPause();
        if (IsFullAnalysisRunning()) AdvanceFullAnalysis();
    }
}
