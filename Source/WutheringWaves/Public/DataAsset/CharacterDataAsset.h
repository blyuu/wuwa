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

	// HUD icon for this ability shown on the skill bar
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UGameplayEffect> DamageEffect;
	
	UPROPERTY(EditDefaultsOnly)
	float DamageMultiplier = 1.f;
	
	UPROPERTY(EditDefaultsOnly)
	float Cooldown = 0.f;


};

// Dodge has one input but three montages picked by situation, so it gets its own
// block instead of the Skills map (which is one-montage-per-tag and carries damage data).
USTRUCT(BlueprintType)
struct FDodgeData
{
	GENERATED_BODY()

	// Played when an enemy is attacking nearby (perfect dodge).
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> PerfectMontage;

	// Played when a movement key is held: dash in the input direction.
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> ForwardMontage;

	// Played with no movement input: dash backward.
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> BackMontage;
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

	// The three dodge montages for this character (perfect / forward / back).
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	FDodgeData Dodge;

	
	// To give different hp per character -> Later will use functions to give that
	UPROPERTY(EditDefaultsOnly)
	float MaxHp = 100.f;
	
	UPROPERTY(EditDefaultsOnly)
	int32 Level = 1;
	
};
