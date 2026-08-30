// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WuwaEnemyController.generated.h"

/**
 * 
 */
UCLASS()
class WUTHERINGWAVES_API AWuwaEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	// runs the Behavior Tree held by the possessed enemy
	virtual void OnPossess(APawn* InPawn) override;
};
