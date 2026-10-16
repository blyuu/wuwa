#include "AnimNotify/AN_WeaponTrail.h"
#include "Character/BaseCharacter.h"
#include "Character/WeaponClass.h"
#include "Components/SkeletalMeshComponent.h"
#include "Effects/WeaponTrailComponent.h"

void UAN_WeaponTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
    ABaseCharacter* Character = MeshComp ? Cast<ABaseCharacter>(MeshComp->GetOwner()) : nullptr;
    if (!Character || !Character->CurrentWeapon || !TrailSystem || !TrailMaterial) return;
    // A loop/restart of this same window must not leave an old sampler running.
    TInlineComponentArray<UWeaponTrailComponent*> PreviousTrails(Character);
    for (UWeaponTrailComponent* Previous : PreviousTrails)
        if (Previous->BelongsTo(this)) Previous->Finish();
    UWeaponTrailComponent* Trail = NewObject<UWeaponTrailComponent>(Character);
    Trail->RegisterComponent();
    if (!Trail->Start(Character->CurrentWeapon->SkeletalMeshComponent, TrailSystem, TrailMaterial,
        this, WidthScale, TotalDuration * 2.f + 1.f))
    {
        UE_LOG(LogTemp, Warning, TEXT("Weapon trail requires WeaponRoot/WeaponTip sockets on %s"), *Character->GetName());
        Trail->DestroyComponent();
    }
}

void UAN_WeaponTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);
    if (!MeshComp || !MeshComp->GetOwner()) return;
    TInlineComponentArray<UWeaponTrailComponent*> Trails(MeshComp->GetOwner());
    for (UWeaponTrailComponent* Trail : Trails)
        if (Trail->BelongsTo(this)) Trail->Finish();
}
