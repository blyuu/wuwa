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

	// BehaviorTree의 공격 Task(BTTask_EnemyAttack)가 호출한다.
	// 현재 페이즈에 맞는 스킬을 골라 CurrentSkillTag에 세팅하고 GA_EnemyAttack을 발동.
	void PerformAttack();

	// 현재 HP 비율로 페이즈를 계산한다 (1부터 시작). EnemyDataAsset->PhaseHpRatios 기준.
	int32 GetCurrentPhase() const;

	// PerformAttack이 고른 공격 타입 태그. GA_EnemyAttack이 이 태그로 데이터 에셋에서 GE/배율을 찾는다.
	FGameplayTag CurrentSkillTag;

	// PerformAttack이 그 태그의 몽타주 목록에서 랜덤으로 뽑은 몽타주. GA_EnemyAttack이 이걸 재생한다.
	UPROPERTY()
	TObjectPtr<class UAnimMontage> CurrentMontage;

protected:
	virtual void HandleDeath() override;
};
