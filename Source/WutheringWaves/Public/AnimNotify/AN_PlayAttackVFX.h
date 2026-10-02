#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_PlayAttackVFX.generated.h"

class UNiagaraSystem;

/** Plays a one-shot attack effect at the exact frame marked on an attack montage. */
UCLASS(meta = (DisplayName = "Play Attack VFX"))
class WUTHERINGWAVES_API UAN_PlayAttackVFX : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// Assign a Niagara burst in the montage notify's Details panel.
	UPROPERTY(EditAnywhere, Category = "Attack VFX")
	TObjectPtr<UNiagaraSystem> Effect;

	// Use the equipped weapon mesh, or the character mesh for unarmed/ranged attacks.
	UPROPERTY(EditAnywhere, Category = "Attack VFX")
	bool bAttachToWeapon = true;

	// Leave empty to attach to the chosen mesh origin; otherwise use one of its sockets.
	UPROPERTY(EditAnywhere, Category = "Attack VFX")
	FName SocketName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Attack VFX")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Attack VFX")
	FRotator RotationOffset = FRotator::ZeroRotator;
};
