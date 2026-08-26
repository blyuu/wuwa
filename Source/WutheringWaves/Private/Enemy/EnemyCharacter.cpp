// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "DataAsset/EnemyDataAsset.h"

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
}

void AEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();
	Destroy();
}
