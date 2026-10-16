#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AN_WeaponTrail.generated.h"

class UNiagaraSystem;
class UMaterialInterface;

UCLASS(meta=(DisplayName="Weapon Blade Trail"))
class WUTHERINGWAVES_API UAN_WeaponTrail : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Trail") TObjectPtr<UNiagaraSystem> TrailSystem;
    UPROPERTY(EditAnywhere, Category="Trail") TObjectPtr<UMaterialInterface> TrailMaterial;
    UPROPERTY(EditAnywhere, Category="Trail", meta=(ClampMin="0.1", ClampMax="2.0")) float WidthScale = 1.15f;
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
