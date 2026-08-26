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
	
	
	void BeginPlay() override;
	
	
protected:
	virtual void HandleDeath() override;
};
