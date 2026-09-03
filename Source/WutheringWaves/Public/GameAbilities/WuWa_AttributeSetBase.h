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
	
	UPROPERTY()
	FGameplayAttributeData Stamina;
	
	UPROPERTY()
	FGameplayAttributeData MaxStamina;
	
	UPROPERTY()
	FGameplayAttributeData Damage;

	// 공격력 배수 (1.0 = 버프 없음). Support characters (수수) raise this on the whole team for a while,
	// and every outgoing hit is multiplied by the attacker's value in PostGameplayEffectExecute.
	UPROPERTY()
	FGameplayAttributeData AttackPower;

	// Groggy / stagger gauge (bosses). Starts full (MaxGroggy), drains when hit; boss staggers at 0.
	UPROPERTY()
	FGameplayAttributeData Groggy;

	UPROPERTY()
	FGameplayAttributeData MaxGroggy;

	// 공명회로 - powers each character's own resonance skill (shown on the overlay bar above HP)
	UPROPERTY()
	FGameplayAttributeData ResonanceEnergy;

	UPROPERTY()
	FGameplayAttributeData MaxResonanceEnergy;

	// 궁극기 효율 게이지 - fills up to enable the ultimate/liberation
	UPROPERTY()
	FGameplayAttributeData UltimateEnergy;

	UPROPERTY()
	FGameplayAttributeData MaxUltimateEnergy;

	// 변주 게이지 (the donut circuit) - fills on hit; full -> charged intro swap
	UPROPERTY()
	FGameplayAttributeData VariationEnergy;

	UPROPERTY()
	FGameplayAttributeData MaxVariationEnergy;

public:
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, Hp);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, MaxHp);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, Damage);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, AttackPower);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, Groggy);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, MaxGroggy);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, ResonanceEnergy);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, MaxResonanceEnergy);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, UltimateEnergy);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, MaxUltimateEnergy);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, VariationEnergy);
	ATTRIBUTE_ACCESSORS(UWuWa_AttributeSetBase, MaxVariationEnergy);
	
	
	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	
};
