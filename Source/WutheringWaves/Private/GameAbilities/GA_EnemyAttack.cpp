// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GA_EnemyAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "DataAsset/EnemyDataAsset.h"
#include "Enemy/EnemyCharacter.h"
#include "GameplayTags/WuwaGameplayTags.h"

UGA_EnemyAttack::UGA_EnemyAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_EnemyAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(ActorInfo->AvatarActor);
	if (!Enemy || !Enemy->EnemyDataAsset)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// just play the montage PerformAttack picked from the tag list and set
	if (!Enemy->CurrentMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentMontage = Enemy->CurrentMontage;

	UAbilityTask_PlayMontageAndWait* PlayAttackMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CurrentMontage);
	PlayAttackMontageTask->OnBlendOut.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->ReadyForActivation();

	// wait for the enemy only hit event fired by the weapon notify (WeaponAnimNotifyState) on the enemy montage
	// the notify's HitEventTag must be set to Event.EnemyAttack.Hit for it to arrive here
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_EnemyAttack_Hit);
	WaitHit->EventReceived.AddDynamic(this, &UGA_EnemyAttack::OnHitEvent);
	WaitHit->ReadyForActivation();
}

void UGA_EnemyAttack::OnHitEvent(FGameplayEventData Payload)
{
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!Enemy || !Enemy->EnemyDataAsset) return;

	// look up GE/multiplier by the attack type tag PerformAttack picked
	const FEnemySkillData* SkillData = Enemy->EnemyDataAsset->Skills.Find(Enemy->CurrentSkillTag);
	if (!SkillData || !SkillData->DamageEffect) return;

	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	if (!HitActor) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!TargetASC || !SourceASC) return;

	UE_LOG(LogTemp, Warning, TEXT("[EnemyAttack] Hit confirmed: %s"), *HitActor->GetName());

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();

	// pass DamageMultiplier as the spec level if the GE's damage scales with level
	// each monster can tune its attack power just by changing the multiplier in the data asset
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(SkillData->DamageEffect, SkillData->DamageMultiplier, Context);
	if (Spec.IsValid())
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void UGA_EnemyAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	CurrentMontage = nullptr;
}

void UGA_EnemyAttack::EndMontage()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
