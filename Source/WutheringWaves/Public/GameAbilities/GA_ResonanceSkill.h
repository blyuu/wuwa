// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_ResonanceSkill.generated.h"

class UAnimMontage;
class UGameplayEffect;

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

	// blocks re-cast while the cooldown tag is present on the owner
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// Cooldown effect applied on cast (its duration = FSkillData.Cooldown via SetByCaller). Assign GE_CoolDown in the BP.
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffect;

	// Tag granted while on cooldown - blocks re-cast and is what the HUD queries. Set per ability BP
	// (Cooldown.ResonanceSkill for the skill, Cooldown.Liberation for the ultimate).
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;

	UFUNCTION()
	void EndMontage();

	UFUNCTION()
	void OnMoveFinished();

private:
	// receives the weapon notify's hit event (Event.Attack.Hit) and applies this skill's damage
	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);

	// fired by an AnimNotify on the skill montage (Event.Skill.Voice) -> plays one random voice line
	UFUNCTION()
	void OnVoiceEvent(FGameplayEventData Payload);

	void PlaySkillVoice();

	TObjectPtr<UAnimMontage> CurrentMontage = nullptr;
};
