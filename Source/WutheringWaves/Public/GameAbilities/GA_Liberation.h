// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameAbilities/GA_ResonanceSkill.h"
#include "GA_Liberation.generated.h"

/**
 * Ultimate (Resonance Liberation).
 *
 * The cast flow is identical to a resonance skill (inherited from UGA_ResonanceSkill): it plays the
 * montage mapped to its own asset tag (Ability.Type.Liberation), soft-locks the nearest enemy, and
 * applies that entry's damage / groggy / variation on the weapon-notify hit event.
 *
 * Ultimate-specific rule added here: it can only be cast when the ultimate gauge (UltimateEnergy) is
 * FULL, and casting drains it back to 0.
 */
UCLASS()
class WUTHERINGWAVES_API UGA_Liberation : public UGA_ResonanceSkill
{
	GENERATED_BODY()

public:
	// gate: only allow activation while the ultimate gauge is full
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// consume the gauge, then run the shared skill flow
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
