#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AeroAttackFXLibrary.generated.h"

class UNiagaraSystem;
class UMaterialInterface;

UCLASS()
class WUTHERINGWAVES_API UAeroAttackFXLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Call only after gameplay confirms a hit. Does not perform damage or hit detection. */
    UFUNCTION(BlueprintCallable, Category="VFX|Aero", meta=(WorldContext="WorldContextObject"))
    static void SpawnAeroConfirmedHit(const UObject* WorldContextObject, UNiagaraSystem* ImpactSystem, FVector HitLocation, float Scale = 1.f);

    /** GroundLocation/Normal must come from a floor trace, not from the character capsule center. */
    UFUNCTION(BlueprintCallable, Category="VFX|Aero", meta=(WorldContext="WorldContextObject"))
    static void SpawnAeroGroundGust(const UObject* WorldContextObject, UNiagaraSystem* GustSystem,
        UMaterialInterface* GustMaterial, FVector GroundLocation, FVector GroundNormal, float Radius = 180.f);
};
