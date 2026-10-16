#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "ParryCameraShake.generated.h"

UCLASS()
class WUTHERINGWAVES_API UParryCameraShake : public UCameraShakeBase
{
    GENERATED_BODY()
public:
    UParryCameraShake(const FObjectInitializer& ObjectInitializer);
};
