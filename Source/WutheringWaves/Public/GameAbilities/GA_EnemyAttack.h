// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_EnemyAttack.generated.h"

class UAnimMontage;

UCLASS()
class WUTHERINGWAVES_API UGA_EnemyAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EnemyAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void EndMontage();

private:
	// 적은 콤보/입력이 없으므로 플레이어 GA_BaseAttack에서 콤보 로직을 걷어낸 단순 버전이다.
	TObjectPtr<UAnimMontage> CurrentMontage = nullptr;

	UFUNCTION()
	void OnHitEvent(FGameplayEventData Payload);
};
