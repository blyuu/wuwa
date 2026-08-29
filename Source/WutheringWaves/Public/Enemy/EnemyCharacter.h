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

	// 임시 확인용: N초마다 GA_EnemyAttack을 발동시켜 동작을 눈으로 검증한다.
	// 이후 AIController/BehaviorTree로 교체할 예정.
	void PerformAttack();

	FTimerHandle AttackTimerHandle;
};
