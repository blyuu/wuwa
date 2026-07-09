#include "GameAbilities/GA_BaseAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimationAsset.h"
#include "Character/BaseCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Character/WeaponClass.h"

UGA_BaseAttack::UGA_BaseAttack()
{
	
	
}

void UGA_BaseAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ActorInfo->AvatarActor);
	
	BaseCharacter->CurrentWeapon->ShowWeapon();
	
	UE_LOG(LogTemp, Display, TEXT("Used BaseAttack"));
	
	const FGameplayTag SkillTag  = GetAssetTags().First();	
	
	const FSkillData* Skill = BaseCharacter->CharacterData->Skills.Find(SkillTag);
	if (!Skill || !Skill->Montage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	UAbilityTask_PlayMontageAndWait* MontageTask =
	 UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		 this, NAME_None, Skill->Montage);
	// OnCompleted/OnBlendOut/OnInterrupted/OnCancelled → EndMontage 바인딩 (기존과 동일)
	MontageTask->ReadyForActivation();
	
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UGA_BaseAttack::EndMontage);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_BaseAttack::EndMontage);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_BaseAttack::EndMontage);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_BaseAttack::EndMontage);
		MontageTask->ReadyForActivation();
	}
	
}

void UGA_BaseAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ActorInfo->AvatarActor);
	
	BaseCharacter->CurrentWeapon->HideWeapon();
	
}

void UGA_BaseAttack::EndMontage()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	
}
