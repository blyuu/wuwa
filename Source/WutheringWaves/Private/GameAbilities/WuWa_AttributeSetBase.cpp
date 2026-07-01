// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/WuWa_AttributeSetBase.h"

UWuWa_AttributeSetBase::UWuWa_AttributeSetBase()
{
	InitHp(100.f);
	InitMaxHp(100.f);
}

bool UWuWa_AttributeSetBase::PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void UWuWa_AttributeSetBase::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}

void UWuWa_AttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void UWuWa_AttributeSetBase::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}
