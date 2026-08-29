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

	// 적 스킬은 데이터 에셋의 Skills 맵에서 자기 태그로 찾아온다 (플레이어와 동일한 룩업 방식).
	const FEnemySkillData* SkillData = Enemy->EnemyDataAsset->Skills.Find(EnemyAbilityTags::Enemy_Ability_Attack);
	if (!SkillData || !SkillData->Montage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentMontage = SkillData->Montage;

	UAbilityTask_PlayMontageAndWait* PlayAttackMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CurrentMontage);
	PlayAttackMontageTask->OnBlendOut.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->ReadyForActivation();

	// 히트 판정은 플레이어와 같은 무기 노티파이(WeaponAnimNotifyState)가 쏘는 Event.BaseAttack.Hit을 재사용한다.
	// 이벤트는 각 캐릭터의 ASC에만 발생하므로 플레이어/적이 같은 태그를 써도 서로 간섭하지 않는다.
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_BaseAttack_Hit);
	WaitHit->EventReceived.AddDynamic(this, &UGA_EnemyAttack::OnHitEvent);
	WaitHit->ReadyForActivation();
}

void UGA_EnemyAttack::OnHitEvent(FGameplayEventData Payload)
{
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!Enemy || !Enemy->EnemyDataAsset) return;

	const FEnemySkillData* SkillData = Enemy->EnemyDataAsset->Skills.Find(EnemyAbilityTags::Enemy_Ability_Attack);
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
