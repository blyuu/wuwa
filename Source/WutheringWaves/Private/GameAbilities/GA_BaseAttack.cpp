#include "GameAbilities/GA_BaseAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimationAsset.h"
#include "Character/BaseCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Character/WeaponClass.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_BaseAttack::UGA_BaseAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
}

void UGA_BaseAttack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	UE_LOG(LogTemp, Warning, TEXT("[Combo] InputPressed - window open: %s"), bWindowIsOpen ? TEXT("true") : TEXT("false"));

	if (bWindowIsOpen)
	{
		bWindowIsOpen = false;
		AdvanceCombo();
	}
	else
	{
		bComboInputBuffered = true;
	}
}

void UGA_BaseAttack::AdvanceCombo()
{
	CurrentComboIndex++;

	FName CurrentSection = FName(*FString::Printf(TEXT("Attack_%d"), CurrentComboIndex));
	FName NextSection    = FName(*FString::Printf(TEXT("Attack_%d"), CurrentComboIndex + 1));

	UE_LOG(LogTemp, Warning, TEXT("[Combo] %s -> %s"), *CurrentSection.ToString(), *NextSection.ToString());

	UAnimInstance* AnimInstance = GetCurrentActorInfo()->GetAnimInstance();

	if (CurrentMontage->GetSectionIndex(NextSection) != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(NextSection, CurrentMontage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combo] Section NOT found - no more combos"));
	}
}

void UGA_BaseAttack::OnComboWindowOpen(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Warning, TEXT("[Combo] Window opened - buffered: %s"), bComboInputBuffered ? TEXT("true") : TEXT("false"));

	if (bComboInputBuffered)
	{
		bComboInputBuffered = false;
		AdvanceCombo();
	}
	else
	{
		bWindowIsOpen = true;
	}
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
	bWindowIsOpen = false;

	// 콤보 윈도우 이벤트 대기
	UAbilityTask_WaitGameplayEvent* WaitCombo = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Combo_WindowOpen);
	WaitCombo->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnComboWindowOpen);
	WaitCombo->ReadyForActivation();

	// 피격 판정 이벤트 대기
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_BaseAttack_Hit);
	WaitHit->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnHitEvent);
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
	bWindowIsOpen = false;
}

void UGA_BaseAttack::EndMontage()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}


void UGA_BaseAttack::OnHitEvent(FGameplayEventData Payload)
{
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!BaseCharacter || !BaseCharacter->CharacterData) return;

	if (BaseCharacter->CharacterData->RangeTag == RangeTags::Character_Range_Ranged)
	{
		PerformRangedTrace();
	}
	else
	{
		// 근접: WeaponAnimNotifyState에서 sweep 후 payload에 대상 담아 이벤트 발송
		AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
		if (HitActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Melee] Hit confirmed: %s"), *HitActor->GetName());
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
			
			FGameplayTag BaseAttackTag = GetAssetTags().First();
			TSubclassOf<UGameplayEffect> DamageEffectClass = BaseCharacter->CharacterData->Skills[BaseAttackTag].DamageEffect;
			
			if (TargetASC && DamageEffectClass)
			{
				FGameplayEffectContextHandle Context = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
				
				FGameplayEffectSpecHandle Spec = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
				
				TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}
}

void UGA_BaseAttack::PerformRangedTrace()
{
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetCurrentActorInfo()->AvatarActor);

	const APlayerController* PC = Cast<APlayerController>(BaseCharacter->GetController());
	if (!PC)
	{
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const FVector TraceEnd = CameraLocation + CameraRotation.Vector() * 5000.f;

	FHitResult HitResult;
	TArray<AActor*> ActorsToIgnore = { BaseCharacter };

	UKismetSystemLibrary::LineTraceSingle(
		BaseCharacter,
		CameraLocation,
		TraceEnd,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResult,
		true
	);

	if (HitResult.bBlockingHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Ranged] Hit: %s"), *HitResult.GetActor()->GetName());
	}
}

FGameplayTag UGA_BaseAttack::GetComboTargetEventTag()
{
	return FGameplayTag::RequestGameplayTag("Event.BaseAttack.Hit");
}

