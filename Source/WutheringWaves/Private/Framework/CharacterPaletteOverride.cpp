#include "Framework/CharacterPaletteOverride.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

ACharacterPaletteOverride::ACharacterPaletteOverride()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACharacterPaletteOverride::BeginPlay()
{
	Super::BeginPlay();
	RefreshPalette();
	GetWorldTimerManager().SetTimer(RefreshTimer, this, &ACharacterPaletteOverride::RefreshPalette, 0.25f, true);
}

void ACharacterPaletteOverride::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RefreshTimer);
	RestorePalette();
	Super::EndPlay(EndPlayReason);
}

void ACharacterPaletteOverride::RefreshPalette()
{
	ACharacter* Character = UGameplayStatics::GetPlayerCharacter(this, 0);
	USkeletalMeshComponent* Mesh = IsValid(Character) ? Character->GetMesh() : nullptr;
	if (!IsValid(Mesh) || Mesh->GetSkeletalMeshAsset() != ExpectedMesh ||
		!FaceMaterial || !ClothUpMaterial || !ClothDownMaterial || Mesh->GetNumMaterials() < 5)
	{
		RestorePalette();
		return;
	}

	if (AppliedMesh.Get() == Mesh)
	{
		return;
	}

	RestorePalette();
	OriginalFace = Mesh->GetMaterial(2);
	OriginalClothUp = Mesh->GetMaterial(3);
	OriginalClothDown = Mesh->GetMaterial(4);
	Mesh->SetMaterial(2, FaceMaterial);
	Mesh->SetMaterial(3, ClothUpMaterial);
	Mesh->SetMaterial(4, ClothDownMaterial);
	AppliedMesh = Mesh;
}

void ACharacterPaletteOverride::RestorePalette()
{
	if (USkeletalMeshComponent* Mesh = AppliedMesh.Get())
	{
		Mesh->SetMaterial(2, OriginalFace);
		Mesh->SetMaterial(3, OriginalClothUp);
		Mesh->SetMaterial(4, OriginalClothDown);
	}
	AppliedMesh.Reset();
	OriginalFace = nullptr;
	OriginalClothUp = nullptr;
	OriginalClothDown = nullptr;
}
