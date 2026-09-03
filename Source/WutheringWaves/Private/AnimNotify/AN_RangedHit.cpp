// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify/AN_RangedHit.h"

#include "AbilitySystemComponent.h"
#include "Character/PlayableCharacter.h"
#include "Enemy/EnemyCharacter.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

void UAN_RangedHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	APlayableCharacter* Character = Cast<APlayableCharacter>(MeshComp->GetOwner());
	if (!Character)
	{
		return;
	}

	// find the enemy in front (reuses the soft-lock target finder: nearest in the camera cone + range)
	AEnemyCharacter* Target = Character->AcquireTargetEnemy();
	if (!Target)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// fire the same hit channel a weapon swing uses; the ability's OnHitEvent applies the damage
	FGameplayEventData EventData;
	EventData.Instigator = Character;
	EventData.Target     = Target;
	ASC->HandleGameplayEvent(EventTags::Event_Attack_Hit, &EventData);
}

