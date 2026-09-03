// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GE_AttackBuff.h"

#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "GameplayTags/WuwaGameplayTags.h"

UGE_AttackBuff::UGE_AttackBuff()
{
	// duration-based team buff; the actual seconds are injected per-cast via SetByCaller (Data.BuffDuration)
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationSetByCaller;
	DurationSetByCaller.DataTag = DataTags::Data_BuffDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationSetByCaller);

	// AttackPower += SetByCaller(Data.AttackBuff). AttackPower's base is 1.0, so +0.3 -> x1.3 outgoing damage.
	// It's additive so two buffs stack cleanly, and GAS restores 1.0 automatically when the effect expires.
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWuWa_AttributeSetBase::GetAttackPowerAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat MagnitudeSetByCaller;
	MagnitudeSetByCaller.DataTag = DataTags::Data_AttackBuff;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MagnitudeSetByCaller);

	Modifiers.Add(Modifier);

	// re-casting refreshes the timer on the same character instead of stacking another +0.3
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
}
