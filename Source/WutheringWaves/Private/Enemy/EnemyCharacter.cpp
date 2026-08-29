// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "DataAsset/EnemyDataAsset.h"
#include "Enemy/WuwaEnemyController.h"
#include "GameplayTags/WuwaGameplayTags.h"

AEnemyCharacter::AEnemyCharacter()
{
	// possess 배관을 C++에 박아 BP 세팅 실수를 원천 차단한다.
	// (지난번 "부여가 안 됨"이 possess 미발생 때문이었던 걸 방지)
	AIControllerClass = AWuwaEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
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
