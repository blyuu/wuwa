#include "GraphicsSettingManager.h"
#include "GraphicsOptimizerRuntime.h"

float FGraphicsSettingManager::GetValue(const Scalability::FQualityLevels& Q, EGraphicsSetting S)
{
    switch (S)
    {
    case EGraphicsSetting::ResolutionScale: return Q.ResolutionQuality;
    case EGraphicsSetting::Shadow: return Q.ShadowQuality;
    case EGraphicsSetting::GlobalIllumination: return Q.GlobalIlluminationQuality;
    case EGraphicsSetting::Reflection: return Q.ReflectionQuality;
    case EGraphicsSetting::PostProcess: return Q.PostProcessQuality;
    case EGraphicsSetting::Texture: return Q.TextureQuality;
    case EGraphicsSetting::Effects: return Q.EffectsQuality;
    }
    return 0.0f;
}

void FGraphicsSettingManager::SetValue(Scalability::FQualityLevels& Q, EGraphicsSetting S, float Value)
{
    switch (S)
    {
    case EGraphicsSetting::ResolutionScale: Q.ResolutionQuality = Value; break;
    case EGraphicsSetting::Shadow: Q.ShadowQuality = FMath::RoundToInt(Value); break;
    case EGraphicsSetting::GlobalIllumination: Q.GlobalIlluminationQuality = FMath::RoundToInt(Value); break;
    case EGraphicsSetting::Reflection: Q.ReflectionQuality = FMath::RoundToInt(Value); break;
    case EGraphicsSetting::PostProcess: Q.PostProcessQuality = FMath::RoundToInt(Value); break;
    case EGraphicsSetting::Texture: Q.TextureQuality = FMath::RoundToInt(Value); break;
    case EGraphicsSetting::Effects: Q.EffectsQuality = FMath::RoundToInt(Value); break;
    }
}

const TCHAR* FGraphicsSettingManager::GetSettingName(EGraphicsSetting S)
{
    switch (S)
    {
    case EGraphicsSetting::ResolutionScale: return TEXT("Resolution Scale");
    case EGraphicsSetting::Shadow: return TEXT("Shadow Quality");
    case EGraphicsSetting::GlobalIllumination: return TEXT("Global Illumination Quality");
    case EGraphicsSetting::Reflection: return TEXT("Reflection Quality");
    case EGraphicsSetting::PostProcess: return TEXT("Post Process Quality");
    case EGraphicsSetting::Texture: return TEXT("Texture Quality");
    case EGraphicsSetting::Effects: return TEXT("Effects Quality");
    }
    return TEXT("Unknown");
}

bool FGraphicsSettingManager::CaptureOriginal()
{
    Original = Scalability::GetQualityLevels();
    bHasOriginal = true;
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Settings] Captured current settings: resolution=%.1f shadow=%d GI=%d reflection=%d post=%d texture=%d effects=%d"),
        Original.ResolutionQuality, Original.ShadowQuality, Original.GlobalIlluminationQuality,
        Original.ReflectionQuality, Original.PostProcessQuality, Original.TextureQuality, Original.EffectsQuality);
    return true;
}

bool FGraphicsSettingManager::ApplyCandidate(const FGraphicsExperimentCandidate& Candidate, Scalability::FQualityLevels& OutApplied)
{
    if (!bHasOriginal)
    {
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Settings] Cannot apply candidate: no original snapshot."));
        return false;
    }
    Scalability::FQualityLevels Requested = Original;
    if (Candidate.Setting == EGraphicsSetting::ResolutionScale)
    {
        if (Candidate.Value < 10.0f || Candidate.Value > 100.0f)
        {
            UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Settings] Invalid resolution scale %.1f; expected 10..100."), Candidate.Value);
            return false;
        }
        SetValue(Requested, Candidate.Setting, Candidate.Value);
    }
    else
    {
        const int32 Level = FMath::RoundToInt(Candidate.Value);
        if (Level < 0 || Level > 4 || !FMath::IsNearlyEqual(Candidate.Value, static_cast<float>(Level)))
        {
            UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Settings] Invalid quality level %.1f; expected integer 0..4."), Candidate.Value);
            return false;
        }
        SetValue(Requested, Candidate.Setting, Candidate.Value);
    }
    Scalability::SetQualityLevels(Requested);
    OutApplied = Scalability::GetQualityLevels();
    const float Actual = GetValue(OutApplied, Candidate.Setting);
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Settings] %s: %.1f -> requested %.1f, applied %.1f"),
        GetSettingName(Candidate.Setting), GetValue(Original, Candidate.Setting), Candidate.Value, Actual);
    if (!FMath::IsNearlyEqual(Actual, Candidate.Value, 0.1f))
    {
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Settings] Requested setting was not applied."));
        return false;
    }
    return true;
}

bool FGraphicsSettingManager::ApplyConfiguration(const Scalability::FQualityLevels& Requested,
    Scalability::FQualityLevels& OutApplied)
{
    if (!bHasOriginal) return false;
    Scalability::SetQualityLevels(Requested);
    OutApplied = Scalability::GetQualityLevels();
    if (!(OutApplied == Requested))
    {
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Settings] Combined configuration was not applied exactly."));
        return false;
    }
    return true;
}

bool FGraphicsSettingManager::RestoreOriginal()
{
    if (!bHasOriginal) return true;
    UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Settings] Restoring original graphics configuration."));
    Scalability::SetQualityLevels(Original);
    const bool bRestored = Scalability::GetQualityLevels() == Original;
    if (bRestored)
    {
        UE_LOG(LogGraphicsOptimizer, Log, TEXT("[GraphicsOptimizer][Settings] Restore succeeded."));
    }
    else
    {
        UE_LOG(LogGraphicsOptimizer, Error, TEXT("[GraphicsOptimizer][Settings] Restore failed."));
    }
    return bRestored;
}
