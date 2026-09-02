// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/WeaponAnimNotifyState.h"
#include "Character/BaseCharacter.h"
#include "Character/WeaponClass.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"

void UWeaponAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);

	HitActors.Empty();

	ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner());
	if (!Character || !Character->CurrentWeapon) return;

	USkeletalMeshComponent* WeaponMesh = Character->CurrentWeapon->SkeletalMeshComponent;
	PrevSocketRoot = WeaponMesh->GetSocketLocation(AWeaponClass::SocketWeaponRoot);
	PrevSocketTip  = WeaponMesh->GetSocketLocation(AWeaponClass::SocketWeaponTip);
}

void UWeaponAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);

	ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner());
	if (!Character || !Character->CurrentWeapon) return;

	USkeletalMeshComponent* WeaponMesh = Character->CurrentWeapon->SkeletalMeshComponent;
	const FVector CurrSocketRoot = WeaponMesh->GetSocketLocation(AWeaponClass::SocketWeaponRoot);
	const FVector CurrSocketTip  = WeaponMesh->GetSocketLocation(AWeaponClass::SocketWeaponTip);

	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore = { Character };

	// sweep from prev tip -> current tip (the blade tip draws the widest arc)
	UKismetSystemLibrary::SphereTraceMulti(
		MeshComp,
		PrevSocketTip,
		CurrSocketTip,
		TraceRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true
	);

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActors.Contains(HitActor)) continue;

		HitActors.Add(HitActor);

		UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
		if (!ASC) continue;

		FGameplayEventData EventData;
		FGameplayAbilityTargetData_SingleTargetHit* TargetData = new FGameplayAbilityTargetData_SingleTargetHit(Hit);
		EventData.TargetData.Add(TargetData);
		EventData.Instigator = Character;
		EventData.Target     = HitActor;

		// fire with the tag set on the montage if not set fall back to the player attack tag
		const FGameplayTag EventToSend = HitEventTag.IsValid() ? HitEventTag : EventTags::Event_Attack_Hit;
		ASC->HandleGameplayEvent(EventToSend, &EventData);
	}

	PrevSocketRoot = CurrSocketRoot;
	PrevSocketTip  = CurrSocketTip;
}

void UWeaponAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::NotifyEnd(MeshComp, Animation);

	HitActors.Empty();
	PrevSocketRoot = FVector::ZeroVector;
	PrevSocketTip  = FVector::ZeroVector;
}
