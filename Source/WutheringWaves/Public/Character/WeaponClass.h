#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponClass.generated.h"

class UWeaponDataAsset;

UCLASS()
class WUTHERINGWAVES_API AWeaponClass : public AActor
{
	GENERATED_BODY()
	
public:
	AWeaponClass();

	static const FName SocketWeaponRoot;
	static const FName SocketWeaponTip;

	// Weapon Data Assets is here to set the Default settings of Weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UWeaponDataAsset> WeaponData;
	
	// Weapon Collider
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UBoxComponent> Collider;
	
	//Skeletal Mesh Component of the Weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;
	

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	void ShowWeapon();
	
	void HideWeapon();

};
