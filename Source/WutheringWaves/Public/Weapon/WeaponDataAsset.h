// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "WeaponDataAsset.generated.h"

UCLASS()
class WUTHERINGWAVES_API UWeaponDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	
	//Weapon ID, Weapon Name
	UPROPERTY(EditDefaultsOnly)
	FName WeaponID;
	UPROPERTY(EditDefaultsOnly)
	FText WeaponName;
	
	
	//Weapon BaseAttack
	UPROPERTY(EditDefaultsOnly)
	int32 BaseAttack;
	
	//Weapon Default Stats
	UPROPERTY(EditDefaultsOnly)
	float SubStats;
	
	//Weapon Type Tags
	UPROPERTY(EditDefaultsOnly,meta = (Categories = "Weapon.Type"))
	FGameplayTag WeaponType;
	
	
	//Weapon Skeletal Mesh Settings
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USkeletalMesh> SkeletalMesh;
	
	
	
};
