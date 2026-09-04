// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TeamComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "Character/WeaponClass.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Enemy/EnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/WuwaHUD.h"

// Sets default values for this component's properties
UTeamComponent::UTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTeamComponent::InitializeTeam()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	SpawnedTeam.Empty();

	// Spawn at the level's PlayerStart so moving it in the editor actually works.
	// Falls back to the fixed SpawnLocation if the level has no PlayerStart.
	FVector SpawnLoc = SpawnLocation;
	FRotator SpawnRot = FRotator::ZeroRotator;
	if (AGameModeBase* GameMode = World->GetAuthGameMode())
	{
		if (AActor* StartSpot = GameMode->FindPlayerStart(PC))
		{
			SpawnLoc = StartSpot->GetActorLocation();
			SpawnRot = StartSpot->GetActorRotation();
		}
	}

	// Spawn all the characters first
	for (int32 i = 0; i < TeamRoster.Num(); ++i)
	{
		if (!TeamRoster[i])
		{
			continue;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APlayableCharacter* NewChar = World->SpawnActor<APlayableCharacter>(
			TeamRoster[i], SpawnLoc, SpawnRot, Params);

		if (!NewChar)
		{
			continue;
		}

		SpawnedTeam.Add(NewChar);

		
		DeActivateCharacter(NewChar);
	}

	if (SpawnedTeam.Num() == 0)
	{
		return;
	}

	// Index 0 Character is first Activated
	ActiveIndex = 0;
	ActivateCharacter(SpawnedTeam[0]);
	PC->Possess(SpawnedTeam[0]);

	// Point the HUD at the starting character (no-op if the HUD isn't up yet - it seeds itself in BeginPlay)
	if (AWuwaHUD* HUD = Cast<AWuwaHUD>(PC->GetHUD()))
	{
		HUD->OnPlayerCharacterChanged(SpawnedTeam[0]);
	}
}

void UTeamComponent::SwitchCharacter(int32 Index)
{
	
	// ignore new switch requests while a charged swap's outro is still playing out
	if (PendingSwapIndex != INDEX_NONE)
	{
		return;
	}

	if (!SpawnedTeam.IsValidIndex(Index) || Index == ActiveIndex)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	APlayableCharacter* Outgoing = SpawnedTeam[ActiveIndex];
	APlayableCharacter* Incoming = SpawnedTeam[Index];

	if (!Incoming)
	{
		return;
	}

	// charged swap: outro (outgoing lingers ~1s and attacks) -> then swap + intro
	if (IsVariationFull(Outgoing))
	{
		PerformChargedSwapEffect(Outgoing);   // nearest-enemy 5% fixed damage + consume the gauge

		// outro: the outgoing character does a basic attack while it lingers
		if (UAbilitySystemComponent* ASC = Outgoing->GetAbilitySystemComponent())
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTags::Ability_Type_BaseAttack));
		}

		// delay the real swap; FinishChargedSwap does the swap + the incoming's GA_Intro (flank appear)
		PendingSwapIndex = Index;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ChargedSwapTimer, this, &UTeamComponent::FinishChargedSwap, ChargedSwapOutroTime, false);
		}
		return;
	}

	// normal instant swap
	DoSwap(Index);
}

void UTeamComponent::DoSwap(int32 Index)
{
	if (!SpawnedTeam.IsValidIndex(Index))
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	APlayableCharacter* Outgoing = SpawnedTeam[ActiveIndex];
	APlayableCharacter* Incoming = SpawnedTeam[Index];
	if (!Incoming)
	{
		return;
	}

	// keep the camera facing across the swap
	const FRotator SavedControlRotation = PC->GetControlRotation();

	if (Outgoing)
	{
		Incoming->SetActorLocationAndRotation(Outgoing->GetActorLocation(), Outgoing->GetActorRotation());
		DeActivateCharacter(Outgoing);
	}

	ActivateCharacter(Incoming);
	PC->Possess(Incoming);
	PC->SetControlRotation(SavedControlRotation);
	ActiveIndex = Index;

	// Rebind the HUD to the newly active character
	if (AWuwaHUD* HUD = Cast<AWuwaHUD>(PC->GetHUD()))
	{
		HUD->OnPlayerCharacterChanged(Incoming);
	}

	// incoming character's swap line (cuts any swap voice still playing so they don't overlap)
	PlaySwapVoice(Incoming);
}

void UTeamComponent::PlaySwapVoice(APlayableCharacter* Char)
{
	// stop the previous swap voice if it's still playing
	if (CurrentVoiceComp)
	{
		CurrentVoiceComp->Stop();
		CurrentVoiceComp = nullptr;
	}

	if (Char && Char->CharacterData && Char->CharacterData->SwapVoice)
	{
		CurrentVoiceComp = UGameplayStatics::SpawnSoundAttached(Char->CharacterData->SwapVoice, Char->GetMesh());
	}
}

void UTeamComponent::FinishChargedSwap()
{
	const int32 Index = PendingSwapIndex;
	PendingSwapIndex = INDEX_NONE;

	if (!SpawnedTeam.IsValidIndex(Index))
	{
		return;
	}

	DoSwap(Index);

	// intro: the incoming character flanks the enemy + plays its intro (GA_Intro repositions it)
	if (APlayableCharacter* Incoming = SpawnedTeam[Index])
	{
		if (UAbilitySystemComponent* ASC = Incoming->GetAbilitySystemComponent())
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTags::Ability_Type_Intro));
		}
	}
}

void UTeamComponent::ActivateCharacter(APlayableCharacter* Char)
{
	if (!Char)
	{
		return;
	}

	Char->SetActorHiddenInGame(false);
	Char->SetActorEnableCollision(true);
	Char->SetActorTickEnabled(true);

	if (UCharacterMovementComponent* Movement = Char->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
}

void UTeamComponent::DeActivateCharacter(APlayableCharacter* Char)
{
	if (!Char)
	{
		return;
	}

	Char->SetActorHiddenInGame(true);
	Char->SetActorEnableCollision(false);
	Char->SetActorTickEnabled(false);

	// Stop any montage still playing (e.g. the charged-swap outro attack). Otherwise its weapon-draw notify
	// can fire AFTER we hide the weapon below and re-show it - that's the weapon that "lingers" in the air.
	if (USkeletalMeshComponent* Mesh = Char->GetMesh())
	{
		if (UAnimInstance* Anim = Mesh->GetAnimInstance())
		{
			Anim->Montage_Stop(0.f);
		}
	}

	// The weapon is a separate actor, so hiding the character doesn't hide it. Sheathe it explicitly so a
	// benched character (whose weapon may have been left drawn from an attack) doesn't flash it on swap-in.
	if (Char->CurrentWeapon && !Char->bAlwaysShowWeapon)
	{
		Char->CurrentWeapon->HideWeapon();
	}

	if (UCharacterMovementComponent* Movement = Char->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
}

bool UTeamComponent::IsVariationFull(APlayableCharacter* Char) const
{
	if (!Char)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = Char->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	const float V = ASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetVariationEnergyAttribute());
	const float Max = ASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxVariationEnergyAttribute());
	return Max > 0.f && V >= Max - 0.01f;
}

void UTeamComponent::PerformChargedSwapEffect(APlayableCharacter* Instigator)
{
	if (!Instigator)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = Instigator->GetAbilitySystemComponent();
	if (!SourceASC)
	{
		return;
	}

	// nearest enemy takes MaxHp * percent as fixed damage (via the SetByCaller damage GE)
	if (ChargedSwapDamageEffect)
	{
		if (AEnemyCharacter* Enemy = FindNearestEnemy(Instigator->GetActorLocation()))
		{
			if (UAbilitySystemComponent* TargetASC = Enemy->GetAbilitySystemComponent())
			{
				const float MaxHp = TargetASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxHpAttribute());
				const float Damage = MaxHp * ChargedSwapDamagePercent;

				FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
				FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(ChargedSwapDamageEffect, 1.f, Context);
				if (Spec.IsValid())
				{
					Spec.Data->SetSetByCallerMagnitude(DataTags::Data_Damage, Damage);
					TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				}
			}
		}
	}

	// consume the variation gauge (drives the donut back to empty + stops the blink)
	SourceASC->ApplyModToAttribute(UWuWa_AttributeSetBase::GetVariationEnergyAttribute(), EGameplayModOp::Override, 0.f);
}

AEnemyCharacter* UTeamComponent::FindNearestEnemy(const FVector& From) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyCharacter::StaticClass(), Enemies);

	AEnemyCharacter* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (AActor* Actor : Enemies)
	{
		const float DistSq = FVector::DistSquared(From, Actor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Cast<AEnemyCharacter>(Actor);
		}
	}
	return Best;
}
