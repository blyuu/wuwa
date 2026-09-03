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
