#include "AnimNotify/AN_ParryCue.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

UAN_ParryCue::UAN_ParryCue()
{
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultCue(
        TEXT("/Game/WutheringWaves/VFX/ParryCue/NS_ParryCue.NS_ParryCue"));
    NiagaraSystem = DefaultCue.Object;
}

void UAN_ParryCue::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);
    if (!MeshComp || !NiagaraSystem || !MeshComp->GetWorld()
        || MeshComp->GetWorld()->GetNetMode() == NM_DedicatedServer) return;

    USceneComponent* Anchor = MeshComp;
    FVector WorldCenter;
    const ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());
    UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
    if (Capsule)
    {
        Anchor = Capsule;
        WorldCenter = Capsule->GetComponentLocation() + Capsule->GetUpVector()
            * (Capsule->GetScaledCapsuleHalfHeight() * HeightRatio + HeightOffset);
    }
    else
    {
        // Animation editor preview actors may not be Characters. Use only this
        // skeletal mesh's bounds (not actor bounds that could include weapon/VFX).
        const FBoxSphereBounds BodyBounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
        WorldCenter = BodyBounds.Origin + FVector::UpVector
            * (BodyBounds.BoxExtent.Z * HeightRatio + HeightOffset);
    }

    const FVector LocalCenter = Anchor->GetComponentTransform().InverseTransformPosition(WorldCenter);
    UNiagaraComponent* Cue = UNiagaraFunctionLibrary::SpawnSystemAttached(
        NiagaraSystem, Anchor, NAME_None, LocalCenter, FRotator::ZeroRotator,
        EAttachLocation::KeepRelativeOffset, true, false);
    if (Cue)
    {
        Cue->SetAbsolute(false, false, true);
        Cue->SetWorldScale3D(FVector(FMath::Max(CueScale, 0.01f)));
        Cue->Activate(true);
    }
}

FString UAN_ParryCue::GetNotifyName_Implementation() const
{
    return TEXT("Parry Cue (Body Center)");
}
