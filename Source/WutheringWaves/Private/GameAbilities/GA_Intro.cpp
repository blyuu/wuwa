// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GA_Intro.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/PlayableCharacter.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Enemy/EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Controller.h"

UGA_Intro::UGA_Intro()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Intro::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APlayableCharacter* Character = Cast<APlayableCharacter>(ActorInfo->AvatarActor);
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// appear at the nearest enemy's left / right / behind (random), facing it
	if (AEnemyCharacter* Enemy = FindNearestEnemy(Character->GetActorLocation()))
	{
		const FVector EnemyLoc = Enemy->GetActorLocation();
		const FVector Fwd = Enemy->GetActorForwardVector();
		const FVector Right = Enemy->GetActorRightVector();

		FVector Dir;
		switch (FMath::RandRange(0, 2))
		{
		case 0:  Dir = -Right; break;   // left
		case 1:  Dir =  Right; break;   // right
		default: Dir = -Fwd;   break;   // behind
		}

		FVector NewLoc = EnemyLoc + Dir * FlankDistance;
		NewLoc.Z = Character->GetActorLocation().Z;   // keep the character's height

		FRotator FaceEnemy = (EnemyLoc - NewLoc).Rotation();
		FaceEnemy.Pitch = 0.f;
		FaceEnemy.Roll  = 0.f;

		Character->SetActorLocationAndRotation(NewLoc, FaceEnemy);

		// align the camera to the swapped-in character (look toward the enemy)
		if (AController* C = Character->GetController())
		{
			C->SetControlRotation(FaceEnemy);
		}
	}

	// play the intro montage from the data asset (if any); otherwise just end after repositioning
	UAnimMontage* IntroMontage = Character->CharacterData ? Character->CharacterData->IntroMontage : nullptr;
	if (!IntroMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	CurrentMontage = IntroMontage;

	UAbilityTask_PlayMontageAndWait* PlayIntro = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, IntroMontage);
	PlayIntro->OnBlendOut.AddDynamic(this, &UGA_Intro::EndMontage);
	PlayIntro->OnCancelled.AddDynamic(this, &UGA_Intro::EndMontage);
	PlayIntro->OnCompleted.AddDynamic(this, &UGA_Intro::EndMontage);
	PlayIntro->OnInterrupted.AddDynamic(this, &UGA_Intro::EndMontage);
	PlayIntro->ReadyForActivation();
}

void UGA_Intro::EndMontage()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

AEnemyCharacter* UGA_Intro::FindNearestEnemy(const FVector& From) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyCharacter::StaticClass(), Enemies);

	AEnemyCharacter* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (AActor* Actor : Enemies)
	{
		const float DistSq = FVector::DistSquared(From, Actor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Cast<AEnemyCharacter>(Actor);
		}
	}
	return Best;
}
