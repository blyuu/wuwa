#include "Effects/AeroAttackFXLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
bool CanPlay(const UObject* Context)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
    return World && World->GetNetMode() != NM_DedicatedServer;
}
}

void UAeroAttackFXLibrary::SpawnAeroConfirmedHit(const UObject* Context, UNiagaraSystem* System, FVector Location, float Scale)
{
    if (!System || !CanPlay(Context)) return;
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(Context, System, Location, FRotator::ZeroRotator,
        FVector(FMath::Clamp(Scale, .1f, 4.f)), true, true, ENCPoolMethod::AutoRelease, true);
}

void UAeroAttackFXLibrary::SpawnAeroGroundGust(const UObject* Context, UNiagaraSystem* System,
    UMaterialInterface* Material, FVector Location, FVector Normal, float Radius)
{
    if (!System || !Material || !CanPlay(Context)) return;
    Normal = Normal.GetSafeNormal();
    if (Normal.IsNearlyZero()) Normal = FVector::UpVector;
    FVector U, V;
    Normal.FindBestAxisVectors(U, V);
    const float Size = FMath::Clamp(Radius, 10.f, 350.f);
    U *= Size; V *= Size;
    UNiagaraComponent* FX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(Context, System,
        Location + Normal * 3.f, FRotator::ZeroRotator, FVector::OneVector, true, false, ENCPoolMethod::AutoRelease, false);
    if (!FX) return;
    UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Material, FX);
    const auto Point = [MID](FName Name, FVector P) { MID->SetVectorParameterValue(Name, FLinearColor(P.X,P.Y,P.Z,0)); };
    Point(TEXT("PreviousRoot"), -U-V); Point(TEXT("PreviousTip"), -U+V);
    Point(TEXT("CurrentRoot"), U-V); Point(TEXT("CurrentTip"), U+V);
    FX->SetVariableMaterial(TEXT("User.TrailMaterial"), MID);
    FX->Activate(true);
}
