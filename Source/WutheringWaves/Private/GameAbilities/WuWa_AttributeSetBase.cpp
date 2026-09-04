// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/WuWa_AttributeSetBase.h"

#include "GameplayEffectExtension.h"
#include "Character/BaseCharacter.h"
#include "Character/PlayableCharacter.h"
#include "Enemy/EnemyCharacter.h"
#include "DataAsset/EnemyDataAsset.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Framework/CombatFeedbackSubsystem.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UWuWa_AttributeSetBase::UWuWa_AttributeSetBase()
{
	InitHp(100.f);
	InitMaxHp(100.f);

	// 공격력 배수 starts at 1.0 (no buff). 수수's team buff pushes it up (e.g. 1.3) for its duration.
	InitAttackPower(1.f);

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

		// scale by the attacker's current attack power (1.0 = no buff; 수수's team buff raises it)
		if (UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent())
		{
			const float Atk = SourceASC->GetNumericAttribute(GetAttackPowerAttribute());
			if (Atk > 0.f)
			{
				DmgValue *= Atk;
			}
		}

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
		else if (DmgValue > 0.f)
		{
			// took damage but survived -> play the flinch / hit-react
			if (ABaseCharacter* Character = Cast<ABaseCharacter>(TargetActor))
			{
				Character->PlayHitReact();
			}
		}

		// floating damage number: report to the combat-feedback hub (the HUD listens and pops a screen-space
		// number over the enemy). Only for damage dealt TO enemies - i.e. what the player did.
		if (DmgValue > 0.f)
		{
			if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(TargetActor))
			{
				// impact SFX at the enemy - data-driven, plays on every confirmed hit with no per-montage notify
				if (Enemy->EnemyDataAsset && Enemy->EnemyDataAsset->HitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(Enemy, Enemy->EnemyDataAsset->HitSound, Enemy->GetActorLocation());
				}

				if (UWorld* World = Enemy->GetWorld())
				{
					if (UCombatFeedbackSubsystem* Feedback = World->GetSubsystem<UCombatFeedbackSubsystem>())
					{
						FCombatFeedbackEvent Event;
						Event.Amount = DmgValue;
						Event.WorldLocation = Enemy->GetActorLocation() + FVector(0.f, 0.f, 90.f);

						// attacker's element -> number color
						if (UAbilitySystemComponent* SrcASC = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent())
						{
							if (APlayableCharacter* Attacker = Cast<APlayableCharacter>(SrcASC->GetAvatarActor()))
							{
								if (const UCharacterDataAsset* CharData = Attacker->GetCharacterData())
								{
									Event.ElementTag = CharData->ElementTag;
								}
							}
						}

						Feedback->ReportDamage(Event);
					}
				}
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
