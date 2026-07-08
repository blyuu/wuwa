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
	
	//무기 식별자 ID 및 무기 자체 이름
	UPROPERTY(EditDefaultsOnly)
	FName WeaponID;
	UPROPERTY(EditDefaultsOnly)
	FText WeaponName;
	
	
	//무기 종류 설정용 태그
	UPROPERTY(EditDefaultsOnly,meta = (Categories = "Weapon.Type"))
	FGameplayTag WeaponType;
	
	
	//무기 메쉬 설
	
	
	
	
	
};
