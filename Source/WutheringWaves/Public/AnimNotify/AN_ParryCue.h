#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_ParryCue.generated.h"

class UNiagaraSystem;

/** Parry anticipation anchored to the character's body, independent of mesh origin/sockets. */
UCLASS(meta = (DisplayName = "Parry Cue (Body Center)"))
class WUTHERINGWAVES_API UAN_ParryCue : public UAnimNotify
{
    GENERATED_BODY()

public:
    UAN_ParryCue();
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
    virtual FString GetNotifyName_Implementation() const override;

    UPROPERTY(EditAnywhere, Category = "Parry Cue")
    TObjectPtr<UNiagaraSystem> NiagaraSystem;

    // 0 = capsule center; 1 = capsule top. Scales with each enemy's capsule size.
    UPROPERTY(EditAnywhere, Category = "Parry Cue", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float HeightRatio = 0.15f;

    // Extra height in world centimeters along the capsule's up axis.
    UPROPERTY(EditAnywhere, Category = "Parry Cue", meta = (Units = "cm"))
    float HeightOffset = 0.f;

    UPROPERTY(EditAnywhere, Category = "Parry Cue", meta = (ClampMin = "0.01"))
    float CueScale = 1.f;
};
