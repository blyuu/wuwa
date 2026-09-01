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

	// flat damage this attack deals. Fed straight into the damage GE via SetByCaller (Data.Damage) - no curve table.
	UPROPERTY(EditDefaultsOnly)
	float Damage = 10.f;

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

	// groggy/stagger gauge size (bar under the boss health bar). Starts full at this value and drains to 0.
	UPROPERTY(EditDefaultsOnly)
	float MaxGroggy = 100.f;

	// stagger animation played while groggy. Split it into sections: an intro (collapse/sit-down) that
	// plays once, then the section named below (the "staying down" pose) which loops until recovery.
	UPROPERTY(EditDefaultsOnly, Category = "Groggy")
	TObjectPtr<class UAnimMontage> GroggyMontage;

	// montage section to loop while staggered. Add a section with this name where the "staying down" part
	// begins. If it doesn't exist, the whole montage loops from the start instead.
	UPROPERTY(EditDefaultsOnly, Category = "Groggy")
	FName GroggyLoopSection = TEXT("Loop");

	// get-up / return-to-normal animation played once when groggy fully recovers (AI waits for it to finish)
	UPROPERTY(EditDefaultsOnly, Category = "Groggy")
	TObjectPtr<class UAnimMontage> GroggyRecoverMontage;

	// seconds for groggy to gradually refill 0 -> full after a break (= how long the stagger lasts).
	// only runs while staggered; there's no groggy recovery during normal combat.
	UPROPERTY(EditDefaultsOnly, Category = "Groggy")
	float GroggyRecoverTime = 8.f;

	// incoming damage multiplier while the enemy is groggy
	UPROPERTY(EditDefaultsOnly, Category = "Groggy")
	float GroggyDamageMultiplier = 1.5f;

	// enemy move speed (goes into CharacterMovement MaxWalkSpeed) player default is 600 so enemy usually lower
	UPROPERTY(EditDefaultsOnly)
	float MoveSpeed = 300.f;

	//========================================================================
	// Boss HUD (only used when bIsBoss is true)
	//========================================================================

	// true = show the top-screen boss health bar while this enemy is alive
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	bool bIsBoss = false;

	// name shown above the boss health bar
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	FText DisplayName;

	// level shown next to the boss name
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	int32 Level = 1;

};
