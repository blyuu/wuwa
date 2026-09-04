// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GA_EnemyAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "DataAsset/EnemyDataAsset.h"
#include "Enemy/EnemyCharacter.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

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

	// just play the montage PerformAttack picked from the tag list and set
	if (!Enemy->CurrentMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentMontage = Enemy->CurrentMontage;

	// face the player as the attack commits so the opening frames aren't aimed at air.
	// continuous per-attack tracking during the montage is done by the AN_TrackTarget notify-state.
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(Enemy, 0))
	{
		FVector ToPlayer = Player->GetActorLocation() - Enemy->GetActorLocation();
		ToPlayer.Z = 0.f;
		if (!ToPlayer.IsNearlyZero())
		{
			FRotator Face = ToPlayer.Rotation();
			Face.Pitch = 0.f;
			Face.Roll = 0.f;
			Enemy->SetActorRotation(Face);
		}
	}

	// keep aiming at the player through the wind-up (turned off at the strike in OnHitEvent / on end)
	Enemy->SetAttackTracking(true);

	UAbilityTask_PlayMontageAndWait* PlayAttackMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CurrentMontage);
	PlayAttackMontageTask->OnBlendOut.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyAttack::EndMontage);
	PlayAttackMontageTask->ReadyForActivation();

	// wait for the enemy only hit event fired by the weapon notify (WeaponAnimNotifyState) on the enemy montage
	// the notify's HitEventTag must be set to Event.EnemyAttack.Hit for it to arrive here
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_EnemyAttack_Hit);
	WaitHit->EventReceived.AddDynamic(this, &UGA_EnemyAttack::OnHitEvent);
	WaitHit->ReadyForActivation();

	// voice line event - an AnimNotify (Event.Skill.Voice) on the montage fires this -> plays one random line
	UAbilityTask_WaitGameplayEvent* WaitVoice = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Skill_Voice);
	WaitVoice->EventReceived.AddDynamic(this, &UGA_EnemyAttack::OnVoiceEvent);
	WaitVoice->ReadyForActivation();
}

void UGA_EnemyAttack::OnVoiceEvent(FGameplayEventData Payload)
{
	PlaySkillVoice();
}

void UGA_EnemyAttack::PlaySkillVoice()
{
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!Enemy || !Enemy->EnemyDataAsset) return;

	// pick a random line for the attack type that PerformAttack chose (CurrentSkillTag)
	const FEnemySkillData* SkillData = Enemy->EnemyDataAsset->Skills.Find(Enemy->CurrentSkillTag);
	if (!SkillData || SkillData->VoiceLines.Num() == 0) return;

	const int32 Index = FMath::RandRange(0, SkillData->VoiceLines.Num() - 1);
	if (USoundBase* Voice = SkillData->VoiceLines[Index])
	{
		UGameplayStatics::SpawnSoundAttached(Voice, Enemy->GetMesh());
	}
}

void UGA_EnemyAttack::OnHitEvent(FGameplayEventData Payload)
{
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!Enemy || !Enemy->EnemyDataAsset) return;

	// the strike has landed -> stop aiming so the hit is committed (player can now dodge it)
	Enemy->SetAttackTracking(false);

	// look up GE/multiplier by the attack type tag PerformAttack picked
	const FEnemySkillData* SkillData = Enemy->EnemyDataAsset->Skills.Find(Enemy->CurrentSkillTag);
	if (!SkillData || !SkillData->DamageEffect) return;

	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
	if (!HitActor) return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!TargetASC || !SourceASC) return;

	UE_LOG(LogTemp, Warning, TEXT("[EnemyAttack] Hit confirmed: %s"), *HitActor->GetName());

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();

	// feed the flat damage value straight in via SetByCaller (Data.Damage) - the GE's Damage modifier
	// reads this, so each attack tunes its power just by the Damage field in the data asset (no curve table)
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(SkillData->DamageEffect, 1.f, Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(DataTags::Data_Damage, SkillData->Damage);
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void UGA_EnemyAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// safety: make sure aiming is off when the attack ends (whiff, cancel, groggy, etc.)
	if (ActorInfo)
	{
		if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(ActorInfo->AvatarActor))
		{
			Enemy->SetAttackTracking(false);
		}
	}

	CurrentMontage = nullptr;
}

void UGA_EnemyAttack::EndMontage()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
