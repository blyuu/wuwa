// Fill out your copyright notice in the Description page of Project Settings.



#include "Enemy/EnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DataAsset/EnemyDataAsset.h"
#include "Enemy/WuwaEnemyController.h"
#include "GameAbilities/GA_EnemyAttack.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"

AEnemyCharacter::AEnemyCharacter()
{
	// wire the possess plumbing in C++ so a BP setup mistake can't break it
	// (last time "abilities not granted" was because possess never happened, this prevents that)
	AIControllerClass = AWuwaEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	
	if (EnemyDataAsset)
	{
		InitializeAttributes(EnemyDataAsset->MaxHp);

		// move speed also comes from the data asset (each monster can have its own speed)
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->MaxWalkSpeed = EnemyDataAsset->MoveSpeed;
		}
	}

	// enemy grants abilities here directly instead of relying on AIController possess
	// (bAbilitiesGranted guard means a later PossessedBy won't grant them twice)
	GiveAbilites();
}

int32 AEnemyCharacter::GetCurrentPhase() const
{
	if (!AttributeSet || !EnemyDataAsset)
	{
		return 1;
	}

	const float MaxHpValue = AttributeSet->GetMaxHp();
	if (MaxHpValue <= 0.f)
	{
		return 1;
	}

	const float Ratio = AttributeSet->GetHp() / MaxHpValue;

	// PhaseHpRatios is descending phase goes up by the number of thresholds passed
	int32 Phase = 1;
	for (const float Threshold : EnemyDataAsset->PhaseHpRatios)
	{
		if (Ratio <= Threshold)
		{
			Phase++;
		}
	}
	return Phase;
}

void AEnemyCharacter::PerformAttack()
{
	if (!AbilitySystemComponent || !EnemyDataAsset)
	{
		return;
	}

	const int32 Phase = GetCurrentPhase();

	// step 1: gather attack type tags usable in the current phase (MinPhase <= Phase)
	// skip any type that has no montages
	TArray<FGameplayTag> EligibleTags;
	for (const TPair<FGameplayTag, FEnemySkillData>& Skill : EnemyDataAsset->Skills)
	{
		if (Skill.Value.MinPhase <= Phase && Skill.Value.Montages.Num() > 0)
		{
			EligibleTags.Add(Skill.Key);
		}
	}

	if (EligibleTags.Num() == 0)
	{
		return;
	}

	// pick one tag at random (can extend to distance/weight based later)
	CurrentSkillTag = EligibleTags[FMath::RandRange(0, EligibleTags.Num() - 1)];

	// step 2: pick one montage at random from that tag's montage list
	const FEnemySkillData& Skill = EnemyDataAsset->Skills[CurrentSkillTag];
	CurrentMontage = Skill.Montages[FMath::RandRange(0, Skill.Montages.Num() - 1)];

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] Phase %d -> skill %s"), Phase, *CurrentSkillTag.ToString());

	// find GA_EnemyAttack (or its BP child) by class hierarchy (IsA) and activate it
	// TryActivateAbilityByClass matches the exact class only so it fails for a BP child
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->IsA(UGA_EnemyAttack::StaticClass()))
		{
			AbilitySystemComponent->TryActivateAbility(Spec.Handle);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] GA_EnemyAttack not granted"));
}

void AEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();
	Destroy();
}
