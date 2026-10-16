#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AN_AeroWeaponTrail.generated.h"

class UNiagaraSystem;
class UMaterialInterface;

/** Cosmetic window for existing montages. Each character owns its sampling history. */
UCLASS(meta=(DisplayName="Aero Rover - Sword Wind Trail"))
class WUTHERINGWAVES_API UAN_AeroWeaponTrail : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    UAN_AeroWeaponTrail();
    UPROPERTY(EditAnywhere, Category="Aero") TSoftObjectPtr<UNiagaraSystem> TrailSystem;
    UPROPERTY(EditAnywhere, Category="Aero") TSoftObjectPtr<UMaterialInterface> TrailMaterial;
    /** Translate the entire sword motion into the air in front of the character. */
    UPROPERTY(EditAnywhere, Category="Aero|Air Slash") bool bAirSlash = true;
    /** Character-local offset from the blade midpoint: X forward, Y right, Z up. */
    UPROPERTY(EditAnywhere, Category="Aero|Air Slash", meta=(EditCondition="bAirSlash", Units="cm")) FVector AirOffset = FVector(80.f, 0.f, 0.f);
    UPROPERTY(EditAnywhere, Category="Aero|Air Slash", meta=(EditCondition="bAirSlash", ClampMin="5", ClampMax="200", Units="cm")) float AirBandWidth = 90.f;
    UPROPERTY(EditAnywhere, Category="Aero|Air Slash", meta=(EditCondition="bAirSlash", ClampMin="0.02", ClampMax="3")) float StartWidthScale = .15f;
    UPROPERTY(EditAnywhere, Category="Aero|Air Slash", meta=(EditCondition="bAirSlash", ClampMin="0.02", ClampMax="3")) float EndWidthScale = 2.f;
    /** Below 1 broadens earlier; above 1 keeps the start thin for longer. */
    UPROPERTY(EditAnywhere, Category="Aero|Air Slash", meta=(EditCondition="bAirSlash", ClampMin="0.2", ClampMax="4")) float WidthGrowthExponent = .8f;
    UPROPERTY(EditAnywhere, Category="Aero", meta=(EditCondition="!bAirSlash", ClampMin="0.1", ClampMax="2")) float WidthScale = 1.65f;
    UPROPERTY(EditAnywhere, Category="Aero", meta=(EditCondition="!bAirSlash", ClampMin="-100", ClampMax="150", Units="cm")) float ForwardOffset = 12.f;
    UPROPERTY(EditAnywhere, Category="Aero", meta=(ClampMin="0", Units="s")) float DrawGrace = 0.f;
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
};
