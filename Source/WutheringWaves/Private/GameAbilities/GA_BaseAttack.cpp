#include "GameAbilities/GA_BaseAttack.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimationAsset.h"
#include "Character/BaseCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Character/WeaponClass.h"

UGA_BaseAttack::UGA_BaseAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
}

void UGA_BaseAttack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	bComboInputBuffered = true;
}

void UGA_BaseAttack::OnComboWindowOpen(FGameplayEventData Payload)
{
	if (!bComboInputBuffered) return;
	
	bComboInputBuffered = false;
	
	CurrentComboIndex++;
	
	FName NextSection = FName(*FString::Printf(TEXT("Attack_%d"),CurrentComboIndex+1));
	
	UAnimInstance* AnimInstance = GetCurrentActorInfo()->GetAnimInstance();
	
	if (CurrentMontage-> GetSectionIndex(NextSection) != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(NextSection, CurrentMontage);
	}
}

void UGA_BaseAttack::OnHitEventRecieved(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Display, TEXT("Hit! Combo %d"), CurrentComboIndex +1 );
}

void UGA_BaseAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ActorInfo->AvatarActor);
	
	BaseCharacter->CurrentWeapon->ShowWeapon();
	
	UE_LOG(LogTemp, Display, TEXT("Used BaseAttack"));
	
	FGameplayTag BaseAttackTag = GetAssetTags().First();
	
	if (!BaseCharacter->CharacterData)
	{
		return;
	}
	
	
	//변경 전 UAnimMontage* BaseAttackMontage = BaseCharacter->CharacterData->Skills[BaseAttackTag].Montage;
	
	//변경 후
	CurrentMontage = BaseCharacter->CharacterData->Skills[BaseAttackTag].Montage;
	UAnimMontage* BaseAttackMontage = CurrentMontage;
	
	UAbilityTask_PlayMontageAndWait* PlayBaseAttackMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,BaseAttackMontage);
	
	PlayBaseAttackMontageTask->OnBlendOut.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->OnCancelled.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->OnCompleted.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->OnInterrupted.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->ReadyForActivation();
	
	CurrentComboIndex = 0;
	bComboInputBuffered = false;
	
	// 콤보 윈도우 이벤트 대기
	UAbilityTask_WaitGameplayEvent* WaitCombo = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Combo_WindowOpen);
	WaitCombo->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnComboWindowOpen);
	WaitCombo->ReadyForActivation();

	// 피격 판정 이벤트 대기
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_BaseAttack_Hit);
	WaitHit->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnHitEventRecieved);
	WaitHit->ReadyForActivation();
	
}

void UGA_BaseAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ActorInfo->AvatarActor);
	
	BaseCharacter->CurrentWeapon->HideWeapon();
	
	CurrentComboIndex = 0;
	bComboInputBuffered = false;
	
}

void UGA_BaseAttack::EndMontage()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	
}
