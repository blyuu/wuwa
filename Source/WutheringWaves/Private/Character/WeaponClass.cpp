#include "Character/WeaponClass.h"

#include "Weapon/WeaponDataAsset.h"


AWeaponClass::AWeaponClass()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	RootComponent = SkeletalMeshComponent;
}


void AWeaponClass::BeginPlay()
{
	Super::BeginPlay();
	
	if (WeaponData)
	{
		SkeletalMeshComponent->SetSkeletalMesh(WeaponData->SkeletalMesh);
	}
}


void AWeaponClass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeaponClass::ShowWeapon()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
}

void AWeaponClass::HideWeapon()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

