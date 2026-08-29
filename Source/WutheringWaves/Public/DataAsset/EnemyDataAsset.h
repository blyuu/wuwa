// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "EnemyDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FEnemySkillData
{
	GENERATED_BODY()

	// 이 공격 타입(태그)의 몽타주 변형들. PerformAttack이 여기서 랜덤으로 하나 재생한다.
	// 같은 공격이라도 모션 여러 개 넣어두면 매번 다르게 나온다.
	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<class UAnimMontage>> Montages;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly)
	float DamageMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly)
	float Cooldown = 0.f;

	// 이 스킬을 쓸 수 있는 최소 페이즈. 1이면 처음부터, 2면 2페이즈부터 사용 가능.
	UPROPERTY(EditDefaultsOnly)
	int32 MinPhase = 1;

};

UCLASS()
class WUTHERINGWAVES_API UEnemyDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	//This is the Element that the Enemy has
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Character.Element"))
	FGameplayTag ElementTag;

	//Enemy Type Tag
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Enemy.Type"))
	FGameplayTag EnemyType;

	// 공격 타입(태그)별 스킬. 태그 하나당 몽타주 여러 개를 Montages에 넣는다.
	// key = 공격 타입(Enemy.Ability.*), value = 그 타입의 GE/배율/몽타주 목록.
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Enemy.Ability"))
	TMap<FGameplayTag, FEnemySkillData> Skills;

	// 페이즈 전환 HP 비율. 내림차순으로 넣는다.
	// 예: [0.5]      -> 1페이즈(HP>50%), 2페이즈(HP<=50%)
	//     [0.7, 0.4] -> 1페이즈(>70%), 2페이즈(40~70%), 3페이즈(<=40%)
	// 비우면 항상 1페이즈.
	UPROPERTY(EditDefaultsOnly)
	TArray<float> PhaseHpRatios;

	UPROPERTY(EditDefaultsOnly)
	float MaxHp = 100.f;

	// 적 이동 속도 (CharacterMovement의 MaxWalkSpeed로 들어감). 플레이어 기본이 600이라, 적은 보통 이보다 낮게.
	UPROPERTY(EditDefaultsOnly)
	float MoveSpeed = 300.f;

};
