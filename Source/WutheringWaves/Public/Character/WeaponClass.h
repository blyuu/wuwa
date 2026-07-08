#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponClass.generated.h"

UCLASS()
class WUTHERINGWAVES_API AWeaponClass : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponClass();
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<class USceneComponent> RootScene;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<class USkeletalMesh> SkeletalMeshData;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComp;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
