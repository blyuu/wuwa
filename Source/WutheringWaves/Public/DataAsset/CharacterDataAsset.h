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

	// how much this skill drains from an enemy's groggy gauge on hit
	UPROPERTY(EditDefaultsOnly)
	float GroggyDamage = 0.f;

	// 변주 게이지 (charged-swap circuit) the attacker gains when this skill hits
	UPROPERTY(EditDefaultsOnly)
	float VariationGain = 0.f;

	// voice lines for this skill; one is picked at random and played when the skill is used
	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<class USoundBase>> VoiceLines;

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

	// portrait shown in the right-side team panel
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UTexture2D> Portrait;

	// played by GA_Intro when this character is swapped in via a charged swap (flank appear)
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> IntroMontage;

	// The three dodge montages for this character (perfect / forward / back).
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	FDodgeData Dodge;

	
	// max 변주 게이지 (charged-swap circuit). The gauge fills to this on hits.
	UPROPERTY(EditDefaultsOnly)
	float MaxVariationEnergy = 100.f;

	// movement speeds - hold the dodge key to run (UE default walk is ~600 for reference)
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RunSpeed = 100.f;

	// To give different hp per character -> Later will use functions to give that
	UPROPERTY(EditDefaultsOnly)
	float MaxHp = 100.f;
	
	UPROPERTY(EditDefaultsOnly)
	int32 Level = 1;
	
};
