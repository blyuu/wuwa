// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_TeamBuff.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * Support ability (e.g. 수수's E). Plays the caster's skill montage and applies a timed attack-power
 * buff (GE_AttackBuff) to every member of the team - the benched ones too, so the buff persists across
 * swaps. The buff amount / duration come from the caster's Skills[tag] data (BuffAmount / BuffDuration).
 * Reuses the same cooldown pattern as GA_ResonanceSkill.
 */
UCLASS()
class WUTHERINGWAVES_API UGA_TeamBuff : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TeamBuff();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// blocks re-cast while the cooldown tag is present on the owner
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	// buff GE applied to every teammate; its magnitude/duration are injected from the caster's skill data
	UPROPERTY(EditDefaultsOnly, Category = "Buff")
	TSubclassOf<UGameplayEffect> BuffEffect;

	// cooldown effect applied on cast (duration = FSkillData.Cooldown via SetByCaller). Assign GE_CoolDown in the BP.
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffect;

	// tag granted while on cooldown - blocks re-cast and is what the HUD queries. Set per ability BP.
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;

	UFUNCTION()
	void EndMontage();

	// fired by an AnimNotify on the montage (Event.Skill.Voice) -> plays one random voice line
	UFUNCTION()
	void OnVoiceEvent(FGameplayEventData Payload);

private:
	// apply GE_AttackBuff to every character in the team roster
	void ApplyTeamBuff();

	void PlaySkillVoice();

	TObjectPtr<UAnimMontage> CurrentMontage = nullptr;
};
