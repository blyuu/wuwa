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
	
	//Weapon ID, Weapon Name (ID 및 이름)
	UPROPERTY(EditDefaultsOnly)
	FName WeaponID;
	UPROPERTY(EditDefaultsOnly)
	FText WeaponName;
	
	
	//Weapon BaseAttack (기초 공격력)
	UPROPERTY(EditDefaultsOnly)
	int32 BaseAttack;
	
	//Weapon Default Stats (기초 옵션)
	UPROPERTY(EditDefaultsOnly)
	float SubStats;
	
	//Weapon Type Tags (무기 종류 설정용 태그)
	UPROPERTY(EditDefaultsOnly,meta = (Categories = "Weapon.Type"))
	FGameplayTag WeaponType;
	
	
	//Weapon Skeletal Mesh Settings (스켈레탈 메쉬 설정)
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USkeletalMesh> SkeletalMesh;
	
	
	
};
