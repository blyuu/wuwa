// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "CharacterDataAsset.generated.h"


USTRUCT(BlueprintType)
struct FSkillData
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
class WUTHERINGWAVES_API UCharacterDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Character.Element"))
	FGameplayTag ElementTag;
	
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Ability.Type"))
	TMap<FGameplayTag, FSkillData> Skills;
	
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Character.Range"))
	FGameplayTag RangeTag;
	
	
	// To give different hp per character -> Later will use functions to give that
	UPROPERTY(EditDefaultsOnly)
	float MaxHp = 100.f;
	
	UPROPERTY(EditDefaultsOnly)
	int32 Level = 1;
	
};
