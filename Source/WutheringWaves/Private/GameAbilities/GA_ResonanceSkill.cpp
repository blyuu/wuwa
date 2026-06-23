// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GA_ResonanceSkill.h"

UGA_ResonanceSkill::UGA_ResonanceSkill()
{
}

void UGA_ResonanceSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                         const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	UE_LOG(LogTemp, Display, TEXT("Used Resonance Skill"));
}

void UGA_ResonanceSkill::OnMoveFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
