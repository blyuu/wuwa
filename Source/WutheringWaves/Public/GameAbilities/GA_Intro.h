// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Intro.generated.h"

class UAnimMontage;
class AEnemyCharacter;

/**
 * Charged-swap intro: when a character is swapped in via a full variation gauge, this teleports them to
 * the nearest enemy's left / right / behind (random), faces the enemy, and plays the intro montage.
 */
UCLASS()
class WUTHERINGWAVES_API UGA_Intro : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Intro();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void EndMontage();

protected:
	// how far from the enemy the character appears
	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	float FlankDistance = 250.f;

private:
	TObjectPtr<UAnimMontage> CurrentMontage = nullptr;

	AEnemyCharacter* FindNearestEnemy(const FVector& From) const;
};
