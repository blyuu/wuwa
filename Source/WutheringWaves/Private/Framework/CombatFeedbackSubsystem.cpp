// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CombatFeedbackSubsystem.h"

#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Framework/ParryCameraShake.h"

FParryFeedbackData::FParryFeedbackData()
	: ShakeClass(UParryCameraShake::StaticClass())
{
}

void UCombatFeedbackSubsystem::ReportDamage(const FCombatFeedbackEvent& Event)
{
	OnDamage.Broadcast(Event);
}

void UCombatFeedbackSubsystem::PlayParryFeedback(APawn* ParryingPlayer, AActor* ParriedEnemy, const FParryFeedbackData& Data)
{
	UWorld* World = GetWorld();
	if (!World || !Data.bEnabled || !IsValid(ParryingPlayer) || !IsValid(ParriedEnemy)
		|| World->GetNetMode() != NM_Standalone)
	{
		return;
	}

	APlayerController* PC = ParryingPlayer ? Cast<APlayerController>(ParryingPlayer->GetController()) : nullptr;
	if (PC && Data.ShakeClass)
	{
		PC->ClientStartCameraShake(Data.ShakeClass, Data.ShakeScale);
	}

	// anchor VFX/sound to the capsule center rather than the actor origin, so it lands on the body
	// instead of at the feet
	FVector ImpactLocation = ParriedEnemy ? ParriedEnemy->GetActorLocation() : FVector::ZeroVector;
	if (const ACharacter* EnemyCharacter = Cast<ACharacter>(ParriedEnemy))
	{
		if (const UCapsuleComponent* Capsule = EnemyCharacter->GetCapsuleComponent())
		{
			ImpactLocation = Capsule->GetComponentLocation() + Capsule->GetUpVector()
				* Capsule->GetScaledCapsuleHalfHeight() * Data.ImpactHeightRatio;
		}
	}

	if (Data.ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Data.ImpactEffect, ImpactLocation,
			FRotator::ZeroRotator, FVector(FMath::Max(Data.ImpactScale, 0.01f)));
	}

	if (Data.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, Data.Sound, ImpactLocation);
	}

	StartParrySlowMo(ParryingPlayer, Data);
}

void UCombatFeedbackSubsystem::StartParrySlowMo(APawn* ParryingPlayer, const FParryFeedbackData& Data)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!FMath::IsFinite(Data.Duration) || Data.Duration <= 0.f || !FMath::IsFinite(Data.Dilation)) return;
	if (!bSlowMoActive)
	{
		PreviousWorldDilation = UGameplayStatics::GetGlobalTimeDilation(World);
		bSlowMoActive = true;
	}
	// Restore the previous pawn before taking ownership of a new compensation.
	RestorePlayerSpeed();
	const float Dilation = FMath::Clamp(Data.Dilation, 0.01f, 1.f);
	UGameplayStatics::SetGlobalTimeDilation(World, PreviousWorldDilation * Dilation);
	AppliedWorldDilation = UGameplayStatics::GetGlobalTimeDilation(World);

	SlowMoPlayer = ParryingPlayer;
	PendingEndShakeClass = Data.ShakeClass;
	PendingEndShakeScale = Data.EndShakeScale;

	if (ParryingPlayer)
	{
		PreviousPlayerDilation = ParryingPlayer->CustomTimeDilation;
		AppliedPlayerDilation = Data.bKeepPlayerFast
			? PreviousPlayerDilation * PreviousWorldDilation / FMath::Max(AppliedWorldDilation, UE_SMALL_NUMBER)
			: PreviousPlayerDilation;
		ParryingPlayer->CustomTimeDilation = AppliedPlayerDilation;
	}

	SlowMoEndRealTime = FPlatformTime::Seconds() + Data.Duration;
	if (!ParrySlowMoTicker.IsValid())
		ParrySlowMoTicker = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UCombatFeedbackSubsystem::TickParrySlowMo));
}

bool UCombatFeedbackSubsystem::TickParrySlowMo(float DeltaSeconds)
{
	if (FPlatformTime::Seconds() < SlowMoEndRealTime) return true;
	ParrySlowMoTicker.Reset();
	EndParrySlowMo();
	return false;
}

void UCombatFeedbackSubsystem::RestorePlayerSpeed()
{
	if (SlowMoPlayer.IsValid() && FMath::IsNearlyEqual(SlowMoPlayer->CustomTimeDilation, AppliedPlayerDilation))
		SlowMoPlayer->CustomTimeDilation = PreviousPlayerDilation;
}

void UCombatFeedbackSubsystem::EndParrySlowMo(bool bPlayEndShake)
{
	if (!bSlowMoActive) return;
	if (UWorld* World = GetWorld(); World && FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(World), AppliedWorldDilation))
	{
		UGameplayStatics::SetGlobalTimeDilation(World, PreviousWorldDilation);
	}
	RestorePlayerSpeed();

	if (SlowMoPlayer.IsValid())
	{
		if (bPlayEndShake && PendingEndShakeClass && PendingEndShakeScale > 0.f)
		{
			if (APlayerController* PC = Cast<APlayerController>(SlowMoPlayer->GetController()))
			{
				PC->ClientStartCameraShake(PendingEndShakeClass, PendingEndShakeScale);
			}
		}
	}

	SlowMoPlayer = nullptr;
	PendingEndShakeClass = nullptr;
	bSlowMoActive = false;
}

void UCombatFeedbackSubsystem::Deinitialize()
{
	if (ParrySlowMoTicker.IsValid())
	{
		FTSTicker::RemoveTicker(ParrySlowMoTicker);
		ParrySlowMoTicker.Reset();
	}
	EndParrySlowMo(false);
	OnDamage.Clear();
	Super::Deinitialize();
}
