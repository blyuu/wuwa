#include "Effects/WeaponTrailComponent.h"
#include "Character/WeaponClass.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UWeaponTrailComponent::UWeaponTrailComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // Socket transforms must include the animation and the weapon's attachment update.
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UWeaponTrailComponent::ConfigureAirSlash(FVector InAirOffset, float InBandWidth,
    float InStartScale, float InEndScale, float InGrowthExponent)
{
    bAirSlash = true;
    AirOffset = InAirOffset.GetClampedToMaxSize(250.f);
    AirBandWidth = FMath::Clamp(InBandWidth, 5.f, 200.f);
    StartWidthScale = FMath::Clamp(InStartScale, .02f, 3.f);
    EndWidthScale = FMath::Clamp(InEndScale, .02f, 3.f);
    WidthGrowthExponent = FMath::Clamp(InGrowthExponent, .2f, 4.f);
}

bool UWeaponTrailComponent::Start(USkeletalMeshComponent* InWeapon, UNiagaraSystem* InSystem,
    UMaterialInterface* InMaterial, const UAnimNotifyState* InWindow, float InWidthScale, float InMaxDuration,
    float InForwardOffset, float InDrawGrace)
{
    if (!InWeapon || !InSystem || !InMaterial ||
        !InWeapon->DoesSocketExist(AWeaponClass::SocketWeaponRoot) ||
        !InWeapon->DoesSocketExist(AWeaponClass::SocketWeaponTip)) return false;
    Weapon = InWeapon;
    System = InSystem;
    Material = InMaterial;
    WindowKey = InWindow;
    WidthScale = FMath::Clamp(InWidthScale, 0.1f, 2.f);
    ForwardOffset = FMath::Clamp(InForwardOffset, -100.f, 150.f);
    DrawGrace = FMath::Max(0.f, InDrawGrace);
    MaxDuration = FMath::Max(InMaxDuration, .1f);
    AddTickPrerequisiteComponent(Weapon);
    if (USceneComponent* Parent = Weapon->GetAttachParent()) AddTickPrerequisiteComponent(Parent);
    // First sample on the post-animation tick, not inside NotifyBegin (stale sockets).
    bHasSample = false;
    return true;
}

void UWeaponTrailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    Elapsed += DeltaTime;
    if (!IsValid(Weapon) || !Weapon->GetOwner() || Elapsed > MaxDuration)
    {
        DestroyComponent();
        return;
    }
    // A trail window can begin before the draw-weapon notify (Rover's first
    // swing: 0.069s vs 0.180s). Hidden means pause, not end the window.
    // Discard history so showing the weapon never bridges across a hidden move.
    if (Weapon->GetOwner()->IsHidden())
    {
        bHasSample = false;
        bHasRawSample = false;
        SampleElapsed = 0.f;
        bWasHiddenLastTick = true;
        return;
    }
    if (bWasHiddenLastTick)
    {
        // Just un-sheathed: the draw flourish swings the socket into place too, and
        // without this it gets rendered as if it were part of the attack's trail.
        VisibleGraceRemaining = DrawGrace;
        bWasHiddenLastTick = false;
    }
    if (VisibleGraceRemaining > 0.f)
    {
        VisibleGraceRemaining -= DeltaTime;
        bHasSample = false;
        bHasRawSample = false;
        SampleElapsed = 0.f;
        return;
    }
    SampleElapsed += DeltaTime;
    if (SampleElapsed >= 1.f / 60.f)
    {
        SampleElapsed = FMath::Fmod(SampleElapsed, 1.f / 60.f);
        Sample(); // At most one segment per frame; no burst after a hitch.
    }
}

void UWeaponTrailComponent::Sample()
{
    const FVector BladeRoot = Weapon->GetSocketLocation(AWeaponClass::SocketWeaponRoot);
    const FVector BladeTip = Weapon->GetSocketLocation(AWeaponClass::SocketWeaponTip);
    const FVector BladeDirection = (BladeTip - BladeRoot).GetSafeNormal();
    // "In front of the blade" means ahead along the swing's travel direction, not
    // along the blade's own hilt->tip axis (that only pushed the trail past the tip).
    // Falls back to the blade axis when the tip isn't moving yet (first sample / held pose).
    FVector SweepDirection = BladeDirection;
    if (bHasRawSample)
    {
        const FVector RawDelta = BladeTip - PreviousRawTip;
        if (!RawDelta.IsNearlyZero(0.1f)) SweepDirection = RawDelta.GetSafeNormal();
    }
    PreviousRawTip = BladeTip;
    bHasRawSample = true;
    // Existing trails default to 50cm; new styles can choose a separate offset.
    FVector Root = BladeRoot + SweepDirection * ForwardOffset;
    FVector Tip = Root + (BladeTip - BladeRoot) * WidthScale;
    if (bAirSlash && GetOwner())
    {
        // Translate the entire swept plane, not just the sword tip. Owner-local X
        // points toward the attack, unlike the instantaneous socket travel direction.
        const FVector Center = (BladeRoot + BladeTip) * .5f
            + GetOwner()->GetActorTransform().TransformVectorNoScale(AirOffset);
        const float Growth = FMath::Pow(SlashProgress, WidthGrowthExponent);
        const float Width = FMath::Clamp(AirBandWidth * FMath::Lerp(StartWidthScale, EndWidthScale, Growth), 1.f, 400.f);
        // Keep the approved outer edge path in place and broaden inward. Each
        // saved segment retains its birth width, producing a thin-to-wide sweep.
        Tip = Center + BladeDirection * AirBandWidth * .5f;
        Root = Tip - BladeDirection * Width;
    }
    const float Distance = FMath::Max(FVector::Distance(Root, PreviousRoot), FVector::Distance(Tip, PreviousTip));
    if (bHasSample && Distance < 2.f) return;
    // Do not stretch a trail across a teleport or an animation discontinuity.
    if (bHasSample && Distance < 250.f && FVector::DistSquared(Root, Tip) > 1.f)
    {
        SpawnSegment(PreviousRoot, PreviousTip, Root, Tip, false);
        // Sparse small blade echoes outside the main edge (at most 15 per second).
        if (++SegmentCounter % 4 == 0)
        {
            const FVector Tangent = (Tip - PreviousTip).GetSafeNormal();
            const FVector Normal = FVector::CrossProduct(Tangent, BladeDirection).GetSafeNormal();
            const float Side = (SegmentCounter / 4) % 2 ? 1.f : -1.f;
            const FVector Center = Tip + BladeDirection * (8.f + SegmentCounter % 3 * 5.f) + Normal * Side * 9.f;
            const FVector HalfLength = Tangent * (7.f + SegmentCounter % 3 * 2.f);
            const FVector HalfWidth = BladeDirection * 1.4f;
            SpawnSegment(Center-HalfLength-HalfWidth, Center-HalfLength+HalfWidth,
                Center+HalfLength-HalfWidth, Center+HalfLength+HalfWidth, true);
        }
    }
    PreviousRoot = Root;
    PreviousTip = Tip;
    LastSampleTime = Elapsed;
    bHasSample = true;
}

void UWeaponTrailComponent::SpawnSegment(const FVector& A, const FVector& B, const FVector& C, const FVector& D, bool bShard)
{
        const FVector Origin = (A + B + C + D) * .25f;
        UNiagaraComponent* FX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System,
            Origin, FRotator::ZeroRotator, FVector::OneVector, true, false, ENCPoolMethod::AutoRelease, false);
        if (FX)
        {
            UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Material, FX);
            const auto SetPoint = [MID, &Origin](FName Name, const FVector& Point)
            {
                const FVector Relative = Point - Origin;
                MID->SetVectorParameterValue(Name, FLinearColor(Relative.X, Relative.Y, Relative.Z, 0));
            };
            SetPoint(TEXT("PreviousRoot"), A);
            SetPoint(TEXT("PreviousTip"), B);
            SetPoint(TEXT("CurrentRoot"), C);
            SetPoint(TEXT("CurrentTip"), D);
            MID->SetScalarParameterValue(TEXT("Shard"), bShard ? 1.f : 0.f);
            if (bAirSlash) MID->SetScalarParameterValue(TEXT("BodyWidth"), 1.f);
            MID->SetScalarParameterValue(TEXT("AgeStep"), FMath::Clamp((Elapsed-LastSampleTime)/.45f, 0.f, .3f));
            FX->SetVariableMaterial(TEXT("User.TrailMaterial"), MID);
            FX->Activate(true);
        }
}

void UWeaponTrailComponent::Finish()
{
    // Existing, unattached Niagara segments finish their own 0.45-second fade.
    DestroyComponent();
}
