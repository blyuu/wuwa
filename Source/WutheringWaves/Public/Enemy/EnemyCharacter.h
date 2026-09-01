// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "GameplayTagContainer.h"
#include "EnemyCharacter.generated.h"


UCLASS()
class WUTHERINGWAVES_API AEnemyCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
public:
	AEnemyCharacter();
	
	//Enemy Data
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UEnemyDataAsset> EnemyDataAsset;

	// Behavior Tree this enemy runs the controller (WuwaEnemyController) reads it on possess and runs it
	UPROPERTY(EditAnywhere, Category = "AI")
	TObjectPtr<class UBehaviorTree> BehaviorTree;
	
	
	void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// player skills call this on hit to drain groggy; at 0 the enemy enters the groggy state
	void ApplyGroggyDamage(float Amount);

	// called by the attack Task in the BehaviorTree (BTTask_EnemyAttack)
	// picks a skill for the current phase sets CurrentSkillTag and fires GA_EnemyAttack
	void PerformAttack();

	// works out the phase from current HP ratio (starts at 1) based on EnemyDataAsset->PhaseHpRatios
	int32 GetCurrentPhase() const;

	// attack type tag PerformAttack picked GA_EnemyAttack uses this tag to look up GE/multiplier in the data asset
	FGameplayTag CurrentSkillTag;

	// montage PerformAttack picked at random from that tag's montage list GA_EnemyAttack plays this
	UPROPERTY()
	TObjectPtr<class UAnimMontage> CurrentMontage;

protected:
	virtual void HandleDeath() override;

	// groggy state transitions
	void EnterGroggy();
	void ExitGroggy();            // groggy gauge refilled -> begin recovery (play the get-up montage)
	void FinishGroggyRecovery();  // get-up finished -> resume AI, fully back to normal

	// bound to the get-up montage's end so AI only resumes once the animation completes
	void OnGroggyRecoverMontageEnded(class UAnimMontage* Montage, bool bInterrupted);

	// bIsGroggy: incapacitated (stagger + get-up). bIsRecovering: in the get-up phase (gauge already full).
	bool bIsGroggy = false;
	bool bIsRecovering = false;
};
