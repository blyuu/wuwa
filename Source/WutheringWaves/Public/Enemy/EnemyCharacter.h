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

	// 이 적이 실행할 Behavior Tree. 컨트롤러(WuwaEnemyController)가 possess 시 읽어서 돌린다.
	UPROPERTY(EditAnywhere, Category = "AI")
	TObjectPtr<class UBehaviorTree> BehaviorTree;
	
	
	void BeginPlay() override;

	// BehaviorTree의 공격 Task(BTTask_EnemyAttack)가 호출한다. GA_EnemyAttack을 태그로 발동.
	void PerformAttack();

protected:
	virtual void HandleDeath() override;
};
