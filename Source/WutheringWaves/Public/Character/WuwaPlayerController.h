// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WuwaPlayerController.generated.h"

class UTeamComponent;
class UInputMappingContext;
class UInputAction;

/**
 *
 */
UCLASS()
class WUTHERINGWAVES_API AWuwaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWuwaPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	
	UPROPERTY(VisibleAnywhere, Category = "Team")
	TObjectPtr<UTeamComponent> TeamComponent;

	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> SwitchMappingContext;

	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<TObjectPtr<UInputAction>> SwitchActions;

	void HandleSwitch(int32 Index);
};
