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

	// PerformAttack이 태그 목록에서 뽑아 세팅한 몽타주를 그대로 재생한다.
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

	// 적 몽타주의 무기 노티파이(WeaponAnimNotifyState)가 쏘는 적 전용 히트 이벤트를 기다린다.
	// 노티파이 쪽 HitEventTag를 Event.EnemyAttack.Hit으로 맞춰야 여기로 들어온다.
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_EnemyAttack_Hit);
	WaitHit->EventReceived.AddDynamic(this, &UGA_EnemyAttack::OnHitEvent);
	WaitHit->ReadyForActivation();
}

void UGA_EnemyAttack::OnHitEvent(FGameplayEventData Payload)
{
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!Enemy || !Enemy->EnemyDataAsset) return;

	// PerformAttack이 고른 공격 타입 태그로 GE/배율을 찾는다.
	const FEnemySkillData* SkillData = Enemy->EnemyDataAsset->Skills.Find(Enemy->CurrentSkillTag);
	if (!SkillData || !SkillData->DamageEffect) return;

	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	if (!HitActor) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!TargetASC || !SourceASC) return;

	UE_LOG(LogTemp, Warning, TEXT("[EnemyAttack] Hit confirmed: %s"), *HitActor->GetName());

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();

	// DamageMultiplier를 스펙 레벨로 넘긴다. GE의 데미지 크기가 레벨에 비례하도록 설정돼 있으면
	// 몬스터마다 데이터 에셋에서 배율만 바꿔 공격력을 조절할 수 있다.
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
