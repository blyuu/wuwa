// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_ResonanceSkill.generated.h"

class UAnimMontage;

/**
 * Plays the montage mapped to this ability's own asset tag in the character data (Skills[tag]).
 * The same class serves the resonance skill and the ultimate (Liberation) - each BP just carries a
 * different asset tag - and both reuse the character's soft-lock (FaceTargetForAttack) on activation.
 */
UCLASS()
class WUTHERINGWAVES_API UGA_ResonanceSkill : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ResonanceSkill();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void EndMontage();

	UFUNCTION()
	void OnMoveFinished();

private:
	// receives the weapon notify's hit event (Event.Attack.Hit) and applies this skill's damage
	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	TObjectPtr<UAnimMontage> CurrentMontage = nullptr;
};
