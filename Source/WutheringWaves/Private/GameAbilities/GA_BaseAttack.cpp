#include "GameAbilities/GA_BaseAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimationAsset.h"
#include "Character/BaseCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_BaseAttack::UGA_BaseAttack()
{
	
	
}

void UGA_BaseAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ActorInfo->AvatarActor);
	
	UE_LOG(LogTemp, Display, TEXT("Used BaseAttack"));
	
	
}

void UGA_BaseAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
