// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GA_TeamBuff.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/PlayableCharacter.h"
#include "Character/TeamComponent.h"
#include "DataAsset/CharacterDataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UGA_TeamBuff::UGA_TeamBuff()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_TeamBuff::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                   const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APlayableCharacter* Character = Cast<APlayableCharacter>(ActorInfo->AvatarActor);
	if (!Character || !Character->CharacterData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// this ability plays the montage mapped to its own tag (the character's skill slot)
	const FGameplayTag SkillTag = GetAssetTags().First();
	const FSkillData* Skill = Character->CharacterData->Skills.Find(SkillTag);
	if (!Skill || !Skill->Montage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	CurrentMontage = Skill->Montage;

	UAbilityTask_PlayMontageAndWait* PlayBuff = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CurrentMontage);
	PlayBuff->OnBlendOut.AddDynamic(this, &UGA_TeamBuff::EndMontage);
	PlayBuff->OnCancelled.AddDynamic(this, &UGA_TeamBuff::EndMontage);
	PlayBuff->OnCompleted.AddDynamic(this, &UGA_TeamBuff::EndMontage);
	PlayBuff->OnInterrupted.AddDynamic(this, &UGA_TeamBuff::EndMontage);
	PlayBuff->ReadyForActivation();

	// apply the attack buff to the whole team (self included) right away
	ApplyTeamBuff();

	// voice line event - an AnimNotify on the montage fires this -> plays one random voice line
	UAbilityTask_WaitGameplayEvent* WaitVoice = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Skill_Voice);
	WaitVoice->EventReceived.AddDynamic(this, &UGA_TeamBuff::OnVoiceEvent);
	WaitVoice->ReadyForActivation();

	// apply cooldown (same pattern as GA_ResonanceSkill): duration via SetByCaller + a dynamic cooldown tag
	if (CooldownEffect && CooldownTag.IsValid() && Skill->Cooldown > 0.f)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(CooldownEffect, 1.f, ASC->MakeEffectContext());
			if (Spec.IsValid())
			{
				Spec.Data->SetSetByCallerMagnitude(DataTags::Data_CooldownDuration, Skill->Cooldown);
				Spec.Data->DynamicGrantedTags.AddTag(CooldownTag);
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("Used Team Buff (%s)"), *SkillTag.ToString());
}

bool UGA_TeamBuff::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// blocked while our cooldown tag is on the owner
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC && CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(CooldownTag))
	{
		return false;
	}

	return true;
}

void UGA_TeamBuff::ApplyTeamBuff()
{
	APlayableCharacter* Character = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!Character || !Character->CharacterData) return;

	const FSkillData* Skill = Character->CharacterData->Skills.Find(GetAssetTags().First());
	if (!Skill || !BuffEffect) return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC) return;

	// the team roster lives on the player controller (see UTeamComponent). Buff everyone on it, so a
	// benched character already has the buff when it's swapped in.
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	UTeamComponent* Team = PC ? PC->FindComponentByClass<UTeamComponent>() : nullptr;
	if (!Team) return;

	for (APlayableCharacter* Mate : Team->GetTeam())
	{
		if (!Mate) continue;

		UAbilitySystemComponent* MateASC = Mate->GetAbilitySystemComponent();
		if (!MateASC) continue;

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(BuffEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(DataTags::Data_AttackBuff, Skill->BuffAmount);
			Spec.Data->SetSetByCallerMagnitude(DataTags::Data_BuffDuration, Skill->BuffDuration);
			MateASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void UGA_TeamBuff::OnVoiceEvent(FGameplayEventData Payload)
{
	PlaySkillVoice();
}

void UGA_TeamBuff::PlaySkillVoice()
{
	APlayableCharacter* Character = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!Character || !Character->CharacterData) return;

	const FSkillData* Skill = Character->CharacterData->Skills.Find(GetAssetTags().First());
	if (!Skill || Skill->VoiceLines.Num() == 0) return;

	// pick a random line each time so the same skill doesn't always sound identical
	const int32 Index = FMath::RandRange(0, Skill->VoiceLines.Num() - 1);
	if (USoundBase* Voice = Skill->VoiceLines[Index])
	{
		UGameplayStatics::SpawnSoundAttached(Voice, Character->GetMesh());
	}
}

void UGA_TeamBuff::EndMontage()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
