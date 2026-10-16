#include "Framework/ParryCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UParryCameraShake::UParryCameraShake(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bSingleInstance = true;
    auto Pattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("ParryImpulse"));
    Pattern->Duration = 0.12f;
    Pattern->BlendInTime = 0.01f;
    Pattern->BlendOutTime = 0.09f;
    Pattern->X.Amplitude = 0.3f;
    Pattern->Y.Amplitude = 1.2f;
    Pattern->Z.Amplitude = 0.8f;
    Pattern->LocationFrequencyMultiplier = 24.f;
    Pattern->Pitch.Amplitude = 0.5f;
    Pattern->Yaw.Amplitude = 0.3f;
    Pattern->Roll.Amplitude = 0.15f;
    Pattern->RotationFrequencyMultiplier = 24.f;
    Pattern->FOV.Amplitude = 0.f;
    SetRootShakePattern(Pattern);
}
