#include "AnimNotify/AN_AeroWeaponTrail.h"
#include "Character/BaseCharacter.h"
#include "Character/WeaponClass.h"
#include "Components/SkeletalMeshComponent.h"
#include "Effects/WeaponTrailComponent.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInterface.h"
#include "Animation/AnimNotifyLibrary.h"

UAN_AeroWeaponTrail::UAN_AeroWeaponTrail()
{
    TrailSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/WutheringWaves/VFX/AeroRoverBasic/NS_AeroBladeTrail.NS_AeroBladeTrail")));
    TrailMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/WutheringWaves/VFX/AeroRoverBasic/M_AeroBladeTrail.M_AeroBladeTrail")));
}

void UAN_AeroWeaponTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
    ABaseCharacter* Character = MeshComp ? Cast<ABaseCharacter>(MeshComp->GetOwner()) : nullptr;
    if (!Character || !IsValid(Character->CurrentWeapon) || Character->GetNetMode() == NM_DedicatedServer) return;
    UNiagaraSystem* System = TrailSystem.LoadSynchronous();
    UMaterialInterface* Material = TrailMaterial.LoadSynchronous();
    if (!System || !Material) return;
    TInlineComponentArray<UWeaponTrailComponent*> Previous(Character);
    for (UWeaponTrailComponent* Trail : Previous)
        if (Trail->BelongsTo(this)) Trail->Finish();
    UWeaponTrailComponent* Trail = NewObject<UWeaponTrailComponent>(Character);
    if (bAirSlash) Trail->ConfigureAirSlash(AirOffset, AirBandWidth, StartWidthScale, EndWidthScale, WidthGrowthExponent);
    Trail->SetSlashProgress(UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTimeRatio(EventReference));
    Trail->RegisterComponent();
    if (!Trail->Start(Character->CurrentWeapon->SkeletalMeshComponent, System, Material,
        this, WidthScale, TotalDuration * 2.f + 1.f, ForwardOffset, DrawGrace))
    {
        UE_LOG(LogTemp, Warning, TEXT("Aero trail: missing WeaponRoot/WeaponTip sockets on %s"), *Character->GetName());
        Trail->DestroyComponent();
    }
}

void UAN_AeroWeaponTrail::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
    if (!MeshComp || !MeshComp->GetOwner()) return;
    // Animation time, rather than world elapsed time, follows montage play rate
    // changes and hit-stop. Mutable progress lives on each character's sampler.
    const float Progress = UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTimeRatio(EventReference);
    TInlineComponentArray<UWeaponTrailComponent*> Trails(MeshComp->GetOwner());
    for (UWeaponTrailComponent* Trail : Trails)
        if (Trail->BelongsTo(this)) Trail->SetSlashProgress(Progress);
}

void UAN_AeroWeaponTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
    if (!MeshComp || !MeshComp->GetOwner()) return;
    TInlineComponentArray<UWeaponTrailComponent*> Trails(MeshComp->GetOwner());
    for (UWeaponTrailComponent* Trail : Trails)
        if (Trail->BelongsTo(this)) Trail->Finish();
}
