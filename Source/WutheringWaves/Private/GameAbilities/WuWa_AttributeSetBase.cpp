// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/WuWa_AttributeSetBase.h"

#include "GameplayEffectExtension.h"
#include "Character/BaseCharacter.h"
#include "Enemy/EnemyCharacter.h"
#include "DataAsset/EnemyDataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"

UWuWa_AttributeSetBase::UWuWa_AttributeSetBase()
{
	InitHp(100.f);
	InitMaxHp(100.f);

	// Resonance energy starts empty and builds up during combat
	InitResonanceEnergy(0.f);
	InitMaxResonanceEnergy(100.f);

	// 궁극기 효율 게이지 starts empty
	InitUltimateEnergy(0.f);
	InitMaxUltimateEnergy(100.f);

	// 변주 게이지 (charged swap / donut circuit) starts empty
	InitVariationEnergy(0.f);
	InitMaxVariationEnergy(100.f);

	// Groggy starts full and drains as the boss gets hit; at 0 the boss enters the groggy/stagger state
	// (bosses override these from their data asset)
	InitMaxGroggy(100.f);
	InitGroggy(100.f);
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

		AActor* TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();

		// groggy/staggered enemies take amplified damage (multiplier comes from the enemy's data asset)
		if (Data.Target.HasMatchingGameplayTag(StateTags::Enemy_State_Groggy))
		{
			float Mult = 1.5f;
			if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(TargetActor))
			{
				if (Enemy->EnemyDataAsset)
				{
					Mult = Enemy->EnemyDataAsset->GroggyDamageMultiplier;
				}
			}
			DmgValue *= Mult;
		}

		float OldHp = GetHp();
		float NewHp = FMath::Max(OldHp - DmgValue, 0.f);
		SetHp(NewHp);
		
		if (NewHp <= 0.f)
		{
			if (ABaseCharacter* Character = Cast<ABaseCharacter>(TargetActor))
			{
				Character->HandleDeath();
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("[Damage] %s | HP : %.1f -> %.1f (-%0.1f)"),
			TargetActor? *TargetActor->GetName() : TEXT("Unknown"), OldHp, NewHp, DmgValue);
		
	}
}

void UWuWa_AttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Keep the bar-backing values inside their valid range so the HUD never draws past full/empty
	if (Attribute == GetHpAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHp());
	}
	else if (Attribute == GetResonanceEnergyAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxResonanceEnergy());
	}
	else if (Attribute == GetGroggyAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxGroggy());
	}
	else if (Attribute == GetUltimateEnergyAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxUltimateEnergy());
	}
	else if (Attribute == GetVariationEnergyAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxVariationEnergy());
	}
}

void UWuWa_AttributeSetBase::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}
