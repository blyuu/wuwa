// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "InputActionValue.h"
#include "GameplayTagContainer.h"
#include "PlayableCharacter.generated.h"


UCLASS()
class WUTHERINGWAVES_API APlayableCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	APlayableCharacter();

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

	//=======================================================================================
	//  Character Data like Element,AnimMontage etc 
	//=======================================================================================
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UCharacterDataAsset> CharacterData;
};
