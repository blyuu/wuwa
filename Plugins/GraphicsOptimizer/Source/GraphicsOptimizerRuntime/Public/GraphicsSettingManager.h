#pragma once

#include "GraphicsOptimizerTypes.h"

class GRAPHICSOPTIMIZERRUNTIME_API FGraphicsSettingManager
{
public:
    bool CaptureOriginal();
    bool ApplyCandidate(const FGraphicsExperimentCandidate& Candidate, Scalability::FQualityLevels& OutApplied);
    bool ApplyConfiguration(const Scalability::FQualityLevels& Requested, Scalability::FQualityLevels& OutApplied);
    bool RestoreOriginal();
    const Scalability::FQualityLevels& GetOriginal() const { return Original; }
    bool HasOriginal() const { return bHasOriginal; }
    static float GetValue(const Scalability::FQualityLevels& Quality, EGraphicsSetting Setting);
    static void SetValue(Scalability::FQualityLevels& Quality, EGraphicsSetting Setting, float Value);
    static const TCHAR* GetSettingName(EGraphicsSetting Setting);

private:
    Scalability::FQualityLevels Original;
    bool bHasOriginal = false;
};
