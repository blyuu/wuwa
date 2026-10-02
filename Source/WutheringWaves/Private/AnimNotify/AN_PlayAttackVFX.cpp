#include "AnimNotify/AN_PlayAttackVFX.h"

#include "Character/BaseCharacter.h"
#include "Character/WeaponClass.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

void UAN_PlayAttackVFX::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !Effect)
	{
		return;
	}

	USkeletalMeshComponent* AttachMesh = MeshComp;
	if (bAttachToWeapon)
	{
		const ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner());
		if (!Character || !Character->CurrentWeapon || !Character->CurrentWeapon->SkeletalMeshComponent)
		{
			return;
		}
		AttachMesh = Character->CurrentWeapon->SkeletalMeshComponent;
	}

	UNiagaraFunctionLibrary::SpawnSystemAttached(Effect, AttachMesh, SocketName,
		LocationOffset, RotationOffset, EAttachLocation::KeepRelativeOffset, true);
}
