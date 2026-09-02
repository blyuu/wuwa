// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GE_CoolDown.h"

#include "GameplayTags/WuwaGameplayTags.h"

UGE_CoolDown::UGE_CoolDown()
{
	// duration-based effect; the actual seconds are injected per-cast via SetByCaller
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = DataTags::Data_CooldownDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}

