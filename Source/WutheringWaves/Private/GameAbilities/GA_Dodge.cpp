// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GA_Dodge.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/PlayableCharacter.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Enemy/EnemyCharacter.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

UGA_Dodge::UGA_Dodge()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// A dodge should break out of an ongoing basic-attack combo.
	// (leading :: picks the AbilityTags namespace, not this class's inherited AbilityTags member)
	CancelAbilitiesWithTag.AddTag(::AbilityTags::Ability_Type_BaseAttack);
}

void UGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APlayableCharacter* Character = Cast<APlayableCharacter>(ActorInfo->AvatarActor);
	if (!Character || !Character->CharacterData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Montages are per-character data, pulled from the data asset (same pattern as GA_BaseAttack).
	const FDodgeData& Dodge = Character->CharacterData->Dodge;

	// Movement input still held this frame (world space). Zero == no direction key pressed.
	const FVector MoveInput = Character->GetLastMovementInputVector();

	// Forward dash only when the input points roughly where the camera faces (i.e. pressing W).
	// Sideways, backward, or no input all fall through to the back dash.
	bool bForwardInput = false;
	if (!MoveInput.IsNearlyZero())
	{
		const FRotator ControlYaw(0.f, Character->GetControlRotation().Yaw, 0.f);
		const FVector CameraForward = FRotationMatrix(ControlYaw).GetUnitAxis(EAxis::X);
		const float ForwardDot = FVector::DotProduct(MoveInput.GetSafeNormal(), CameraForward);
		bForwardInput = ForwardDot >= ForwardDotThreshold;
	}

	// --- Decide the situation: perfect > forward > back ---
	bool bPerfectDodge = false;
	if (IsEnemyAttackingNearby())
	{
		CurrentMontage = Dodge.PerfectMontage;
		Character->PlayDodgeSlowMo();   // WuWa-style brief slow-motion on a perfect dodge
		bPerfectDodge = true;
	}
	else if (bForwardInput)
	{
		// Face the input direction so the forward-dash root motion carries us that way.
		FRotator DashRotation = MoveInput.GetSafeNormal().Rotation();
		DashRotation.Pitch = 0.f;
		DashRotation.Roll  = 0.f;
		Character->SetActorRotation(DashRotation);

		CurrentMontage = Dodge.ForwardMontage;
	}
	else
	{
		CurrentMontage = Dodge.BackMontage;
	}

	if (!CurrentMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// dodge whoosh (perfect dodge uses its own sound if one is set, else the normal dodge sound)
	if (USoundBase* DodgeSFX = (bPerfectDodge && Dodge.PerfectDodgeSound) ? Dodge.PerfectDodgeSound.Get() : Dodge.DodgeSound.Get())
	{
		UGameplayStatics::SpawnSoundAttached(DodgeSFX, Character->GetMesh());
	}

	UAbilityTask_PlayMontageAndWait* PlayDodge = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CurrentMontage);
	PlayDodge->OnBlendOut.AddDynamic(this, &UGA_Dodge::EndMontage);
	PlayDodge->OnCancelled.AddDynamic(this, &UGA_Dodge::EndMontage);
	PlayDodge->OnCompleted.AddDynamic(this, &UGA_Dodge::EndMontage);
	PlayDodge->OnInterrupted.AddDynamic(this, &UGA_Dodge::EndMontage);
	PlayDodge->ReadyForActivation();
}

bool UGA_Dodge::IsEnemyAttackingNearby() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return false;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(const_cast<AActor*>(Avatar));

	TArray<AActor*> FoundEnemies;
	UKismetSystemLibrary::SphereOverlapActors(
		Avatar,
		Avatar->GetActorLocation(),
		PerfectDodgeRadius,
		ObjectTypes,
		AEnemyCharacter::StaticClass(),
		IgnoreActors,
		FoundEnemies);

	for (AActor* Enemy : FoundEnemies)
	{
		UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Enemy);
		if (EnemyASC && EnemyASC->HasMatchingGameplayTag(StateTags::Enemy_State_Attacking))
		{
			return true;
		}
	}
	return false;
}

void UGA_Dodge::EndMontage()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	CurrentMontage = nullptr;
}
