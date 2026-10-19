#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CharacterPaletteOverride.generated.h"

class UMaterialInterface;
class USkeletalMesh;
class USkeletalMeshComponent;

/** Applies arena-specific colors only to the currently controlled character mesh. */
UCLASS()
class WUTHERINGWAVES_API ACharacterPaletteOverride : public AActor
{
	GENERATED_BODY()

public:
	ACharacterPaletteOverride();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(EditAnywhere, Category = "Character Palette")
	TObjectPtr<USkeletalMesh> ExpectedMesh;

	UPROPERTY(EditAnywhere, Category = "Character Palette")
	TObjectPtr<UMaterialInterface> FaceMaterial;

	UPROPERTY(EditAnywhere, Category = "Character Palette")
	TObjectPtr<UMaterialInterface> ClothUpMaterial;

	UPROPERTY(EditAnywhere, Category = "Character Palette")
	TObjectPtr<UMaterialInterface> ClothDownMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OriginalFace;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OriginalClothUp;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OriginalClothDown;

	TWeakObjectPtr<USkeletalMeshComponent> AppliedMesh;
	FTimerHandle RefreshTimer;

	void RefreshPalette();
	void RestorePalette();
};
