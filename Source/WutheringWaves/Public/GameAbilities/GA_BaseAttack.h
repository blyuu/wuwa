#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_BaseAttack.generated.h"

class UAnimMontage;

UCLASS()
class WUTHERINGWAVES_API UGA_BaseAttack : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_BaseAttack();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	UFUNCTION()
	void EndMontage();
	
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	
private:
	int32 CurrentComboIndex = 0 ;
	
	bool bComboInputBuffered = false;

	bool bWindowIsOpen = false;
	
	TObjectPtr<UAnimMontage> CurrentMontage = nullptr;
	
	void AdvanceCombo();

	UFUNCTION()
	void OnComboWindowOpen(FGameplayEventData Payload);
	
	UFUNCTION()
	void OnHitEventRecieved(FGameplayEventData Payload);
	
	
};
