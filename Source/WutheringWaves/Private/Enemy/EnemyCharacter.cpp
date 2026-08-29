// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "DataAsset/EnemyDataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "TimerManager.h"

AEnemyCharacter::AEnemyCharacter()
{
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	
	if (EnemyDataAsset)
	{
		InitializeAttributes(EnemyDataAsset->MaxHp);
	}

	// 적은 AIController possess에 의존하지 않고 여기서 직접 어빌리티를 부여한다.
	// (bAbilitiesGranted 가드가 있어 이후 PossessedBy가 겹쳐도 중복 부여되지 않는다)
	GiveAbilites();

	// 임시 확인용: 3초마다 공격을 발동한다. AI 붙일 때 제거 예정.
	GetWorldTimerManager().SetTimer(
		AttackTimerHandle, this, &AEnemyCharacter::PerformAttack, 3.f, true);
}

void AEnemyCharacter::PerformAttack()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// GA_EnemyAttack의 Asset Tag(Enemy.Ability.Attack)로 매칭해 발동한다.
	const bool bActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(
		FGameplayTagContainer(EnemyAbilityTags::Enemy_Ability_Attack));

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] PerformAttack -> activated: %s"),
		bActivated ? TEXT("true") : TEXT("false"));
}

void AEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();
	Destroy();
}
