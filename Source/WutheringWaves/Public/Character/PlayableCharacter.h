// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "InputActionValue.h"
#include "GameplayTagContainer.h"
#include "PlayableCharacter.generated.h"

class AEnemyCharacter;

UCLASS()
class WUTHERINGWAVES_API APlayableCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	APlayableCharacter();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	//=====================================================================================
	// Camera and Input (Only Playable Character)
	//=====================================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UCameraComponent> CameraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class USpringArmComponent> SpringArmComponent;

	// warps an attack montage toward the locked target (a "Motion Warping" notify on the attack montage,
	// warp target name "AttackTarget") so the swing closes the gap and the weapon sweep actually connects
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<class UMotionWarpingComponent> MotionWarping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UWuwaInputConfig> WuwaInputConfig;

	UFUNCTION()
	void InputTagUseAbility(FGameplayTag InputTag);
	
	void AddInputMapping();
	void RemoveInputMapping();

public:
	//========================================================================
	// Character Movements Move,Look, Zoom 
	//========================================================================
	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);
	void MouseWheel(const FInputActionValue& value);

	// hold the dodge key to run (sprint), release to go back to walk speed
	void StartSprint();
	void StopSprint();

	//=======================================================================================
	//  Character Data like Element,AnimMontage etc
	//=======================================================================================
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UCharacterDataAsset> CharacterData;

	// Static level pulled from the data asset (no leveling system) - used by the HUD.
	UFUNCTION(BlueprintPure, Category = "Character")
	int32 GetCharacterLevel() const;

	// The character's data asset (element, skills + their icons, etc.) - used by the HUD.
	UFUNCTION(BlueprintPure, Category = "Character")
	UCharacterDataAsset* GetCharacterData() const { return CharacterData; }

	// playable characters keep everything data-driven: the flinch montage comes from the data asset
	virtual class UAnimMontage* GetHitReactMontage() const override;

	// hit voice lines come from the data asset too
	virtual const TArray<TObjectPtr<class USoundBase>>& GetHitVoiceLines() const override;

	// brief global slow-motion for a perfect dodge (world slows; player optionally stays fast). Auto-restores.
	void PlayDodgeSlowMo();

protected:
	// world time scale during the perfect-dodge slow-mo (0.4 = 40% speed)
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Dodge")
	float DodgeSlowMoScale = 0.4f;

	// how long the slow-mo lasts, in REAL seconds
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Dodge")
	float DodgeSlowMoDuration = 0.35f;

	// keep the player near-normal speed while the world slows (WuWa-style reaction advantage)
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Dodge")
	bool bDodgeSlowMoKeepPlayerFast = true;

public:

	//========================================================================
	// Auto-target ("soft lock"). Attacks / skills / ultimate call FaceTargetForAttack
	// on activation so the character turns toward (and optionally slides to) the enemy
	// it is about to hit. All shared here so every ability reuses one code path.
	//========================================================================

	// nearest live enemy overall (no range / cone filter). Used by the charged-swap intro.
	UFUNCTION(BlueprintPure, Category = "Targeting")
	AEnemyCharacter* FindNearestEnemy() const;

	// best enemy to attack: nearest live one inside the camera-facing cone and within range.
	UFUNCTION(BlueprintPure, Category = "Targeting")
	AEnemyCharacter* AcquireTargetEnemy() const;

	// turn toward the acquired target (+ step in if the data asset asks). Abilities call this
	// once on activation; returns the enemy it locked onto (or null if there was none).
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	AEnemyCharacter* FaceTargetForAttack();

private:
	// restores time dilation when the perfect-dodge slow-mo ends
	FTimerHandle SlowMoTimerHandle;
	void EndDodgeSlowMo();

	// guards one-time attribute init in PossessedBy so a team swap doesn't reset HP/gauges to full
	bool bAttributesInitialized = false;

	// running blend state for the turn / step-in kicked off by FaceTargetForAttack
	bool bTargetAssistActive = false;
	float TargetAssistElapsed = 0.f;
	float TargetAssistBlendTime = 0.f;
	bool bTargetAssistStepIn = false;
	FRotator TargetAssistStartRot;
	FRotator TargetAssistGoalRot;
	FVector TargetAssistStartLoc;
	FVector TargetAssistGoalLoc;
	// orient-to-movement is switched off during the blend so it can't fight our rotation; restored after
	bool bSavedOrientToMovement = true;
};
