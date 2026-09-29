#pragma once

#include "Scalability.h"

class FGraphicsPresetManager
{
public:
    static FString GetPath();
    static bool Save(const Scalability::FQualityLevels& Configuration);
    static bool Load(Scalability::FQualityLevels& OutConfiguration);
};
