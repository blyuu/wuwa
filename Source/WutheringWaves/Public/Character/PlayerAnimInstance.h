// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class WUTHERINGWAVES_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class ABaseCharacter> OwnerCharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCharacterMovementComponent> OwnerCharacterMovement;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector Velocity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float GroundSpeed;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	uint8 bShouldMove : 1;
	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	uint8 bIsFalling: 1;

	// Separate a jump takeoff from stepping off a ledge so each uses the right state.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 bIsJumpingUp: 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 bIsFallingDown: 1;

	// A normal hop lands before this becomes true; keep the fall loop for longer drops.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 bIsLongFall: 1;

	float AirborneTime = 0.f;
	
};
