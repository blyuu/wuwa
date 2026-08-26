// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/WuWa_AttributeSetBase.h"

#include "GameplayEffectExtension.h"
#include "Character/BaseCharacter.h"

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
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float DmgValue = GetDamage();
		SetDamage(0.f);
		
		float OldHp = GetHp();
		float NewHp = FMath::Max(OldHp - DmgValue, 0.f);
		SetHp(NewHp);
		
		if (NewHp <= 0.f)
		{
			AActor* TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
			
			if (ABaseCharacter* Character = Cast<ABaseCharacter>(TargetActor))
			{
				Character->HandleDeath();
			}
		}
		
		AActor* TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		UE_LOG(LogTemp, Warning, TEXT("[Damage] %s | HP : %.1f -> %.1f (-%0.1f)"), 
			TargetActor? *TargetActor->GetName() : TEXT("Unknown"), OldHp, NewHp, DmgValue);
		
	}
}

void UWuWa_AttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void UWuWa_AttributeSetBase::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}
