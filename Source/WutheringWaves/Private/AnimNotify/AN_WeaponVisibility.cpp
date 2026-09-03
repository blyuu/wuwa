// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify/AN_WeaponVisibility.h"

#include "Character/BaseCharacter.h"
#include "Character/WeaponClass.h"
#include "Components/SkeletalMeshComponent.h"

void UAN_WeaponVisibility::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner());
	if (!Character || !Character->CurrentWeapon)
	{
		return;
	}

	if (bShowWeapon)
	{
		Character->CurrentWeapon->ShowWeapon();
	}
	else
	{
		Character->CurrentWeapon->HideWeapon();
	}
}

