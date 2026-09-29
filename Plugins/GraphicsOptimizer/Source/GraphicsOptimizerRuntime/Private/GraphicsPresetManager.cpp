#include "GraphicsPresetManager.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

namespace
{
const TCHAR* Section = TEXT("ScalabilityGroups");

bool IsValidQualityLevel(int32 Value)
{
    return Value >= 0 && Value <= 4;
}
}

FString FGraphicsPresetManager::GetPath()
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GraphicsOptimizer"), TEXT("SelectedScalability.ini"));
}

bool FGraphicsPresetManager::Save(const Scalability::FQualityLevels& Q)
{
    const FString Path = GetPath();
    const FString Directory = FPaths::GetPath(Path);
    if (!IFileManager::Get().MakeDirectory(*Directory, true) &&
        !IFileManager::Get().DirectoryExists(*Directory)) return false;
    FConfigFile Config;
    Config.SetFloat(Section, TEXT("sg.ResolutionQuality"), Q.ResolutionQuality);
    Config.SetInt64(Section, TEXT("sg.ViewDistanceQuality"), Q.ViewDistanceQuality);
    Config.SetInt64(Section, TEXT("sg.AntiAliasingQuality"), Q.AntiAliasingQuality);
    Config.SetInt64(Section, TEXT("sg.ShadowQuality"), Q.ShadowQuality);
    Config.SetInt64(Section, TEXT("sg.GlobalIlluminationQuality"), Q.GlobalIlluminationQuality);
    Config.SetInt64(Section, TEXT("sg.ReflectionQuality"), Q.ReflectionQuality);
    Config.SetInt64(Section, TEXT("sg.PostProcessQuality"), Q.PostProcessQuality);
    Config.SetInt64(Section, TEXT("sg.TextureQuality"), Q.TextureQuality);
    Config.SetInt64(Section, TEXT("sg.EffectsQuality"), Q.EffectsQuality);
    Config.SetInt64(Section, TEXT("sg.FoliageQuality"), Q.FoliageQuality);
    Config.SetInt64(Section, TEXT("sg.ShadingQuality"), Q.ShadingQuality);
    Config.SetInt64(Section, TEXT("sg.LandscapeQuality"), Q.LandscapeQuality);
    return Config.Write(Path);
}

bool FGraphicsPresetManager::Load(Scalability::FQualityLevels& OutConfiguration)
{
    const FString Path = GetPath();
    if (!IFileManager::Get().FileExists(*Path)) return false;
    FConfigFile Config;
    Config.Read(Path);
    Scalability::FQualityLevels Q = Scalability::GetQualityLevels();
    if (!Config.GetFloat(Section, TEXT("sg.ResolutionQuality"), Q.ResolutionQuality) ||
        !Config.GetInt(Section, TEXT("sg.ViewDistanceQuality"), Q.ViewDistanceQuality) ||
        !Config.GetInt(Section, TEXT("sg.AntiAliasingQuality"), Q.AntiAliasingQuality) ||
        !Config.GetInt(Section, TEXT("sg.ShadowQuality"), Q.ShadowQuality) ||
        !Config.GetInt(Section, TEXT("sg.GlobalIlluminationQuality"), Q.GlobalIlluminationQuality) ||
        !Config.GetInt(Section, TEXT("sg.ReflectionQuality"), Q.ReflectionQuality) ||
        !Config.GetInt(Section, TEXT("sg.PostProcessQuality"), Q.PostProcessQuality) ||
        !Config.GetInt(Section, TEXT("sg.TextureQuality"), Q.TextureQuality) ||
        !Config.GetInt(Section, TEXT("sg.EffectsQuality"), Q.EffectsQuality) ||
        !Config.GetInt(Section, TEXT("sg.FoliageQuality"), Q.FoliageQuality) ||
        !Config.GetInt(Section, TEXT("sg.ShadingQuality"), Q.ShadingQuality) ||
        !Config.GetInt(Section, TEXT("sg.LandscapeQuality"), Q.LandscapeQuality))
        return false;

    if (!FMath::IsFinite(Q.ResolutionQuality) ||
        (Q.ResolutionQuality != 0.0f && (Q.ResolutionQuality < 10.0f || Q.ResolutionQuality > 100.0f)) ||
        !IsValidQualityLevel(Q.ViewDistanceQuality) || !IsValidQualityLevel(Q.AntiAliasingQuality) ||
        !IsValidQualityLevel(Q.ShadowQuality) || !IsValidQualityLevel(Q.GlobalIlluminationQuality) ||
        !IsValidQualityLevel(Q.ReflectionQuality) || !IsValidQualityLevel(Q.PostProcessQuality) ||
        !IsValidQualityLevel(Q.TextureQuality) || !IsValidQualityLevel(Q.EffectsQuality) ||
        !IsValidQualityLevel(Q.FoliageQuality) || !IsValidQualityLevel(Q.ShadingQuality) ||
        !IsValidQualityLevel(Q.LandscapeQuality))
        return false;
    OutConfiguration = Q;
    return true;
}
