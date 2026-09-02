#include "GameAbilities/GA_BaseAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimationAsset.h"
#include "Character/BaseCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Character/PlayableCharacter.h"
#include "Character/WeaponClass.h"
#include "Enemy/EnemyCharacter.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "Kismet/GameplayStatics.h"
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
	
	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(ActorInfo->AvatarActor);

	BaseCharacter->CurrentWeapon->ShowWeapon();

	// soft-lock: turn toward (and step in on) the enemy this swing is aimed at
	BaseCharacter->FaceTargetForAttack();

	UE_LOG(LogTemp, Display, TEXT("Used BaseAttack"));
	
	FGameplayTag BaseAttackTag = GetAssetTags().First();
	
	if (!BaseCharacter->CharacterData)
	{
		return;
	}

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

	// Combo Window, Wait for event
	UAbilityTask_WaitGameplayEvent* WaitCombo = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Combo_WindowOpen);
	WaitCombo->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnComboWindowOpen);
	WaitCombo->ReadyForActivation();

	// Take Damage Event
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Attack_Hit);
	WaitHit->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnHitEvent);
	WaitHit->ReadyForActivation();

	// Voice line event - an AnimNotify at each combo section fires this, so every combo hit can voice
	UAbilityTask_WaitGameplayEvent* WaitVoice = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Skill_Voice);
	WaitVoice->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnVoiceEvent);
	WaitVoice->ReadyForActivation();
}

void UGA_BaseAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(ActorInfo->AvatarActor);
	
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
	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!BaseCharacter || !BaseCharacter->CharacterData) return;

	if (BaseCharacter->CharacterData->RangeTag == RangeTags::Character_Range_Ranged)
	{
		PerformRangedTrace();
	}
	else
	{
		
		AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
		if (HitActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Melee] Hit confirmed: %s"), *HitActor->GetName());
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
			
			FGameplayTag BaseAttackTag = GetAssetTags().First();
			const FSkillData& SkillData = BaseCharacter->CharacterData->Skills[BaseAttackTag];
			TSubclassOf<UGameplayEffect> DamageEffectClass = SkillData.DamageEffect;

			if (TargetASC && DamageEffectClass)
			{
				FGameplayEffectContextHandle Context = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();

				FGameplayEffectSpecHandle Spec = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);

				TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}

			// drain the enemy's groggy gauge by this skill's groggy value
			if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(HitActor))
			{
				Enemy->ApplyGroggyDamage(SkillData.GroggyDamage);
			}

			// gain 변주 게이지 (charged-swap circuit) on a confirmed hit
			if (SkillData.VariationGain > 0.f)
			{
				GetAbilitySystemComponentFromActorInfo()->ApplyModToAttribute(
					UWuWa_AttributeSetBase::GetVariationEnergyAttribute(),
					EGameplayModOp::Additive, SkillData.VariationGain);
			}

			// build the 궁극기 효율 게이지 (ultimate) on a confirmed hit
			if (SkillData.UltimateGain > 0.f)
			{
				GetAbilitySystemComponentFromActorInfo()->ApplyModToAttribute(
					UWuWa_AttributeSetBase::GetUltimateEnergyAttribute(),
					EGameplayModOp::Additive, SkillData.UltimateGain);
			}
		}
	}
}

void UGA_BaseAttack::OnVoiceEvent(FGameplayEventData Payload)
{
	PlaySkillVoice();
}

void UGA_BaseAttack::PlaySkillVoice()
{
	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!BaseCharacter || !BaseCharacter->CharacterData) return;

	const FSkillData& Skill = BaseCharacter->CharacterData->Skills[GetAssetTags().First()];
	if (Skill.VoiceLines.Num() == 0) return;

	// pick a random line each time so the same combo doesn't always sound identical
	const int32 Index = FMath::RandRange(0, Skill.VoiceLines.Num() - 1);
	if (USoundBase* Voice = Skill.VoiceLines[Index])
	{
		UGameplayStatics::SpawnSoundAttached(Voice, BaseCharacter->GetMesh());
	}
}

void UGA_BaseAttack::PerformRangedTrace()
{
	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor);

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
	return EventTags::Event_Attack_Hit;
}

