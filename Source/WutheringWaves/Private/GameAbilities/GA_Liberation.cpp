// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GA_Liberation.h"

#include "AbilitySystemComponent.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "GameplayTags/WuwaGameplayTags.h"

UGA_Liberation::UGA_Liberation()
{
	// the whole ultimate is invulnerable (cinematic cast): while active the owner has State.Invulnerable,
	// so incoming damage is rejected and the hit-react can't interrupt the montage. GAS removes it on end.
	ActivationOwnedTags.AddTag(StateTags::State_Invulnerable);
}

bool UGA_Liberation::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return false;
	}

	// gauge must be full to fire the ultimate
	const float Energy = ASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetUltimateEnergyAttribute());
	const float MaxEnergy = ASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxUltimateEnergyAttribute());
	return MaxEnergy > 0.f && Energy >= MaxEnergy - 0.01f;
}

void UGA_Liberation::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// drain the ultimate gauge to 0 on cast (CanActivateAbility already guaranteed it was full)
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ApplyModToAttribute(UWuWa_AttributeSetBase::GetUltimateEnergyAttribute(), EGameplayModOp::Override, 0.f);
	}

	// run the shared resonance-skill flow (montage + soft-lock + hit-damage)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}
