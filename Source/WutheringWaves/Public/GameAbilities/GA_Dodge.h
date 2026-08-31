// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Dodge.generated.h"

class UAnimMontage;

/**
 * One dodge action, three outcomes decided at activation time (priority high -> low):
 *   1. An enemy is attacking nearby   -> perfect dodge montage (i-frame window lives on the montage)
 *   2. A movement key is held         -> forward dash montage, facing the input direction
 *   3. No input                       -> back dash montage
 * Movement is driven by root motion baked into the montages.
 */
UCLASS()
class WUTHERINGWAVES_API UGA_Dodge : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void EndMontage();

protected:
	// The three dodge montages come from the character's data asset (CharacterData->Dodge),
	// so one shared GA_Dodge works for every character.

	// How far to look for an attacking enemy when deciding the perfect dodge.
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float PerfectDodgeRadius = 600.f;

	// Forward dash only when the input direction is within this dot of the camera-forward.
	// 1 = only dead-ahead, 0.5 = within ~60 degrees (front cone), 0 = whole front half.
	// Anything below this (sideways / backward / no input) becomes a back dash.
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	float ForwardDotThreshold = 0.5f;

private:
	TObjectPtr<UAnimMontage> CurrentMontage = nullptr;

	// True if an enemy carrying Enemy.State.Attacking is within PerfectDodgeRadius.
	bool IsEnemyAttackingNearby() const;
};
