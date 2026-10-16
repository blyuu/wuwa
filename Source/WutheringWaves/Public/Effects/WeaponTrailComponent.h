#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponTrailComponent.generated.h"

class UNiagaraSystem;
class UMaterialInterface;
class USkeletalMeshComponent;
class UAnimNotifyState;

/** Per-swing sampling state. Notify assets never store mutable per-character history. */
UCLASS(Transient)
class WUTHERINGWAVES_API UWeaponTrailComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UWeaponTrailComponent();
    bool Start(USkeletalMeshComponent* InWeapon, UNiagaraSystem* InSystem, UMaterialInterface* InMaterial,
        const UAnimNotifyState* InWindow, float InWidthScale, float InMaxDuration,
        float InForwardOffset = 50.f, float InDrawGrace = .15f);
    void Finish();
    /** Offset the swept blade into the air in owner-local space; independent of sword length. */
    void ConfigureAirSlash(FVector InAirOffset, float InBandWidth,
        float InStartScale = 1.f, float InEndScale = 1.f, float InGrowthExponent = 1.f);
    void SetSlashProgress(float Progress) { SlashProgress = FMath::Clamp(Progress, 0.f, 1.f); }
    bool BelongsTo(const UAnimNotifyState* Window) const { return WindowKey == Window; }
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void Sample();
    void SpawnSegment(const FVector& A, const FVector& B, const FVector& C, const FVector& D, bool bShard);
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Weapon;
    UPROPERTY(Transient) TObjectPtr<UNiagaraSystem> System;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Material;
    UPROPERTY(Transient) TObjectPtr<const UAnimNotifyState> WindowKey;
    FVector PreviousRoot = FVector::ZeroVector;
    FVector PreviousTip = FVector::ZeroVector;
    FVector PreviousRawTip = FVector::ZeroVector;
    bool bHasRawSample = false;
    bool bWasHiddenLastTick = true;
    float VisibleGraceRemaining = 0.f;
    float WidthScale = 1.15f;
    bool bAirSlash = false;
    FVector AirOffset = FVector::ZeroVector;
    float AirBandWidth = 90.f;
    float StartWidthScale = 1.f;
    float EndWidthScale = 1.f;
    float WidthGrowthExponent = 1.f;
    float SlashProgress = 0.f;
    float ForwardOffset = 50.f;
    float DrawGrace = .15f;
    float Elapsed = 0;
    float SampleElapsed = 0;
    float MaxDuration = 5;
    bool bHasSample = false;
    int32 SegmentCounter = 0;
    float LastSampleTime = 0.f;
};
