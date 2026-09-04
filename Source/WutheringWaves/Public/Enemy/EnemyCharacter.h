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

	// attack aim: how fast the enemy turns toward the player during an attack wind-up (RInterp speed; 0 = instant)
	UPROPERTY(EditAnywhere, Category = "AI")
	float AttackTurnSpeed = 6.f;

	// stop turning once within this many degrees of the player (avoids jitter)
	UPROPERTY(EditAnywhere, Category = "AI")
	float AttackTrackDeadzoneDeg = 3.f;

	// called by GA_EnemyAttack: while true, the enemy turns toward the player each Tick (aims the wind-up)
	void SetAttackTracking(bool bEnable) { bAttackTracking = bEnable; }

	// rolls the data-asset dodge chance when the player lands a hit. On success plays the backstep montage
	// and returns true (the attacker then skips damage). false = the hit connects normally.
	bool TryDodge();

	// plays the spawn intro (montage + voice) and pauses the AI until it finishes. Called from BeginPlay.
	void PlayIntro();

	// true while the spawn intro is playing - the controller holds combat (paused BT) until this clears
	bool IsPlayingIntro() const { return bIntroPlaying; }
	
	
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

	// enemies only flinch on a chance (EnemyDataAsset->HitReactChance) instead of every hit
	virtual void PlayHitReact() override;

	// looping boss battle music instance (2D). Spawned on encounter start, faded out on death.
	// lives independently of this actor so it keeps playing through the death animation / fade.
	UPROPERTY()
	TObjectPtr<class UAudioComponent> BattleMusicComp;

	// groggy state transitions
	void EnterGroggy();
	void ExitGroggy();            // groggy gauge refilled -> begin recovery (play the get-up montage)
	void FinishGroggyRecovery();  // get-up finished -> resume AI, fully back to normal

	// bound to the get-up montage's end so AI only resumes once the animation completes
	void OnGroggyRecoverMontageEnded(class UAnimMontage* Montage, bool bInterrupted);

	// bIsGroggy: incapacitated (stagger + get-up). bIsRecovering: in the get-up phase (gauge already full).
	bool bIsGroggy = false;
	bool bIsRecovering = false;

	// true while an attack is aiming at the player (set by GA_EnemyAttack around the wind-up)
	bool bAttackTracking = false;

	// rotate yaw toward the current player pawn (used each Tick during attack tracking)
	void FacePlayerYaw(float DeltaTime);

	// true while the backstep dodge montage is playing - prevents re-dodging mid-dodge
	bool bIsDodging = false;

	// clears bIsDodging when the dodge montage finishes
	void OnDodgeMontageEnded(class UAnimMontage* Montage, bool bInterrupted);

	// true while the spawn intro montage is playing
	bool bIntroPlaying = false;

	// resumes the AI when the intro montage finishes
	void OnIntroMontageEnded(class UAnimMontage* Montage, bool bInterrupted);
};
