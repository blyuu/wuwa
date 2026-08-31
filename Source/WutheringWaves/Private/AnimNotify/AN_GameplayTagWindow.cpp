// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify/AN_GameplayTagWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

void UAN_GameplayTagWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);

	if (!MeshComp || !MeshComp->GetOwner() || !TagToApply.IsValid()) return;

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
	{
		ASC->AddLooseGameplayTag(TagToApply);
	}
}

void UAN_GameplayTagWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::NotifyEnd(MeshComp, Animation);

	if (!MeshComp || !MeshComp->GetOwner() || !TagToApply.IsValid()) return;

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
	{
		// Remove one count so nested/overlapping windows of the same tag stay balanced.
		ASC->RemoveLooseGameplayTag(TagToApply);
	}
}
