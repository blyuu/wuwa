// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "EnemyDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FEnemySkillData
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> Montage;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UGameplayEffect> DamageEffect;
	
	UPROPERTY(EditDefaultsOnly)
	float DamageMultiplier = 1.f;
	
	UPROPERTY(EditDefaultsOnly)
	float Cooldown = 0.f;
	
};

UCLASS()
class WUTHERINGWAVES_API UEnemyDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	
	//This is the Element that the Enemy has
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Character.Element"))
	FGameplayTag ElementTag;
	
	//Enemy Type Tag
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Enemy.Type"))
	FGameplayTag EnemyType;
	
	
	UPROPERTY(EditDefaultsOnly)
	float MaxHp = 100.f;
	
};
