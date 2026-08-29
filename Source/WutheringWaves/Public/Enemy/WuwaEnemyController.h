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
	// possess한 적이 들고 있는 Behavior Tree를 실행한다.
	virtual void OnPossess(APawn* InPawn) override;
};
