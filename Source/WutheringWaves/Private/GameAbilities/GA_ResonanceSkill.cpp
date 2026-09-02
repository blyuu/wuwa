// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GA_ResonanceSkill.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/PlayableCharacter.h"
#include "Character/WeaponClass.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Enemy/EnemyCharacter.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "GameplayTags/WuwaGameplayTags.h"

UGA_ResonanceSkill::UGA_ResonanceSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_ResonanceSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                         const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APlayableCharacter* Character = Cast<APlayableCharacter>(ActorInfo->AvatarActor);
	if (!Character || !Character->CharacterData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// same soft-lock the base attack uses: turn toward (and step in on) the target enemy
	Character->FaceTargetForAttack();

	// this ability plays the montage mapped to its own tag (ResonanceSkill or Liberation)
	const FGameplayTag SkillTag = GetAssetTags().First();
	const FSkillData* Skill = Character->CharacterData->Skills.Find(SkillTag);
	if (!Skill || !Skill->Montage)
	{
		// nothing authored for this tag yet - bail out cleanly
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (Character->CurrentWeapon)
	{
		Character->CurrentWeapon->ShowWeapon();
	}

	CurrentMontage = Skill->Montage;

	UAbilityTask_PlayMontageAndWait* PlaySkill = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CurrentMontage);
	PlaySkill->OnBlendOut.AddDynamic(this, &UGA_ResonanceSkill::EndMontage);
	PlaySkill->OnCancelled.AddDynamic(this, &UGA_ResonanceSkill::EndMontage);
	PlaySkill->OnCompleted.AddDynamic(this, &UGA_ResonanceSkill::EndMontage);
	PlaySkill->OnInterrupted.AddDynamic(this, &UGA_ResonanceSkill::EndMontage);
	PlaySkill->ReadyForActivation();

	// wait for the weapon notify's hit event (same channel as the base attack) and apply damage on it.
	// so a skill/ultimate montage only needs a WeaponAnimNotifyState (HitEventTag left empty) to deal damage.
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Attack_Hit);
	WaitHit->EventReceived.AddDynamic(this, &UGA_ResonanceSkill::OnHitEvent);
	WaitHit->ReadyForActivation();

	UE_LOG(LogTemp, Display, TEXT("Used Resonance Skill (%s)"), *SkillTag.ToString());
}

void UGA_ResonanceSkill::OnHitEvent(FGameplayEventData Payload)
{
	APlayableCharacter* Character = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!Character || !Character->CharacterData) return;

	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	if (!HitActor) return;

	// look up this ability's own skill data (ResonanceSkill or Liberation) by its asset tag
	const FGameplayTag SkillTag = GetAssetTags().First();
	const FSkillData* Skill = Character->CharacterData->Skills.Find(SkillTag);
	if (!Skill) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

	// apply the damage GE (same path as the base attack)
	if (SourceASC && TargetASC && Skill->DamageEffect)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Skill->DamageEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// drain the enemy's groggy gauge by this skill's groggy value (0 = no effect)
	if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(HitActor))
	{
		Enemy->ApplyGroggyDamage(Skill->GroggyDamage);
	}

	// gain 변주 게이지 if this skill grants it
	if (SourceASC && Skill->VariationGain > 0.f)
	{
		SourceASC->ApplyModToAttribute(
			UWuWa_AttributeSetBase::GetVariationEnergyAttribute(),
			EGameplayModOp::Additive, Skill->VariationGain);
	}

	// build the 궁극기 효율 게이지 (ultimate) if this skill grants it
	if (SourceASC && Skill->UltimateGain > 0.f)
	{
		SourceASC->ApplyModToAttribute(
			UWuWa_AttributeSetBase::GetUltimateEnergyAttribute(),
			EGameplayModOp::Additive, Skill->UltimateGain);
	}
}

void UGA_ResonanceSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (APlayableCharacter* Character = Cast<APlayableCharacter>(ActorInfo->AvatarActor))
	{
		if (Character->CurrentWeapon)
		{
			Character->CurrentWeapon->HideWeapon();
		}
	}
}

void UGA_ResonanceSkill::EndMontage()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_ResonanceSkill::OnMoveFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
