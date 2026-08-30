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

	// montage variations for this attack type (tag) PerformAttack plays one at random from here
	// even for the same attack putting several motions makes it vary each time
	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<class UAnimMontage>> Montages;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly)
	float DamageMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly)
	float Cooldown = 0.f;

	// min phase this skill can be used 1 = from start 2 = usable from phase 2
	UPROPERTY(EditDefaultsOnly)
	int32 MinPhase = 1;

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

	// skills per attack type (tag) put multiple montages under one tag in Montages
	// key = attack type (Enemy.Ability.*) value = that type's GE/multiplier/montage list
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Enemy.Ability"))
	TMap<FGameplayTag, FEnemySkillData> Skills;

	// phase transition HP ratios put them in descending order
	// ex [0.5]      -> phase1 (HP>50%), phase2 (HP<=50%)
	//    [0.7, 0.4] -> phase1 (>70%), phase2 (40~70%), phase3 (<=40%)
	// leave empty for always phase1
	UPROPERTY(EditDefaultsOnly)
	TArray<float> PhaseHpRatios;

	UPROPERTY(EditDefaultsOnly)
	float MaxHp = 100.f;

	// enemy move speed (goes into CharacterMovement MaxWalkSpeed) player default is 600 so enemy usually lower
	UPROPERTY(EditDefaultsOnly)
	float MoveSpeed = 300.f;

};
