#include "Character/WeaponClass.h"


AWeaponClass::AWeaponClass()
{
	PrimaryActorTick.bCanEverTick = true;
	
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp"));
	
	
	SetRootComponent(RootScene);
	SkeletalMeshComp->SetupAttachment(RootScene);
	
	SkeletalMeshComp->SetSkeletalMesh(SkeletalMeshData);
	
}


void AWeaponClass::BeginPlay()
{
	Super::BeginPlay();
	
}


void AWeaponClass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

