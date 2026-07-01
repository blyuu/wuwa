// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WuWa_AttributeSetBase.generated.h"


#define  ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


UCLASS()
class WUTHERINGWAVES_API UWuWa_AttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UWuWa_AttributeSetBase();
	
private:
	
	UPROPERTY()
	FGameplayAttributeData Hp;
	
	UPROPERTY()
	FGameplayAttributeData MaxHp;
	
public:
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, Hp);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, MaxHp);
	
	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	
};
