// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify/AN_SendTargetGroup.h"

#include "AbilitySystemBlueprintLibrary.h"

void UAN_SendTargetGroup::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	
	// Exception
	if (!MeshComp) return;
	
	if (TargetSocketName.Num() <= 1) return;
	
	if (!MeshComp->GetOwner() || !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()))
	{
		return;
	}
	
	
	FGameplayEventData Data;

	for (int i = 1; i < TargetSocketName.Num(); i++)
	{
		FGameplayAbilityTargetData_LocationInfo* LocationInfo = new FGameplayAbilityTargetData_LocationInfo();

		LocationInfo->SourceLocation.LiteralTransform.SetLocation(MeshComp->GetSocketLocation(TargetSocketName[i - 1]));
		LocationInfo->TargetLocation.LiteralTransform.SetLocation(MeshComp->GetSocketLocation(TargetSocketName[i]));

		Data.TargetData.Add(LocationInfo);
	}

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), EventTag, Data);
}
