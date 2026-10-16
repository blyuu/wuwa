// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "Containers/Ticker.h"
#include "CombatFeedbackSubsystem.generated.h"

/**
 * Data-only payload for one damage instance. Kept minimal on purpose; add crit / DoT flags here later
 * and every listener (the HUD) gets them for free.
 */
USTRUCT()
struct FCombatFeedbackEvent
{
	GENERATED_BODY()

	// damage dealt, already after all multipliers (attack buff, groggy amplification, etc.)
	float Amount = 0.f;

	// world position the number floats up from (usually the victim + a height offset)
	FVector WorldLocation = FVector::ZeroVector;

	// attacker's element -> drives the number color on the HUD
	FGameplayTag ElementTag;
};

// C++-only multicast (the HUD is the sole listener). One param: the event.
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatDamage, const FCombatFeedbackEvent&);

/**
 * WuWa-style "juice" played when a parry lands. Kept entirely data-driven so each enemy
 * (EnemyDataAsset->ParryFeedback) can tune or replace it - a boss can hit harder than a trash mob.
 */
USTRUCT(BlueprintType)
struct FParryFeedbackData
{
	GENERATED_BODY()
	FParryFeedbackData();

	UPROPERTY(EditDefaultsOnly)
	bool bEnabled = true;

	// world time dilation applied while the parry hit-stop plays (1 = no slow, lower = slower)
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float Dilation = 0.1f;

	// how long the slow-mo lasts, in REAL (undilated) seconds
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.01"))
	float Duration = 0.3f;

	// counters the dilation on the parrying player so they read as near-full-speed while the world crawls
	UPROPERTY(EditDefaultsOnly)
	bool bKeepPlayerFast = true;

	// played once immediately, and again when the slow-mo ends - feels snappier than one long shake
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UCameraShakeBase> ShakeClass;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0"))
	float ShakeScale = 1.f;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0"))
	float EndShakeScale = 0.5f;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float ImpactHeightRatio = 0.15f;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.01"))
	float ImpactScale = 1.f;

	// spawned at the parried enemy's capsule center
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UNiagaraSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class USoundBase> Sound;
};

/**
 * Central hub that decouples gameplay from combat UI. The AttributeSet reports every damage instance
 * here (ReportDamage); the HUD subscribes to OnDamage and spawns floating numbers. One world = one hub.
 */
UCLASS()
class WUTHERINGWAVES_API UCombatFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	// gameplay reports a damage instance; broadcasts to every listener
	void ReportDamage(const FCombatFeedbackEvent& Event);

	// the HUD binds to this to pop screen-space damage numbers
	FOnCombatDamage OnDamage;

	// called from GA_BaseAttack::OnHitEvent the instant Enemy->TryParry() succeeds.
	// Handles everything: camera shake, impact VFX/sound at the enemy, and the hit-stop slow-mo.
	// A real-time deadline is renewed on successive parries.
	void PlayParryFeedback(APawn* ParryingPlayer, AActor* ParriedEnemy, const FParryFeedbackData& Data);

private:
	void StartParrySlowMo(APawn* ParryingPlayer, const FParryFeedbackData& Data);
	void EndParrySlowMo(bool bPlayEndShake = true);
	bool TickParrySlowMo(float DeltaSeconds);
	void RestorePlayerSpeed();

	FTSTicker::FDelegateHandle ParrySlowMoTicker;
	double SlowMoEndRealTime = 0.0;
	bool bSlowMoActive = false;
	float PreviousWorldDilation = 1.f;
	float AppliedWorldDilation = 1.f;
	float PreviousPlayerDilation = 1.f;
	float AppliedPlayerDilation = 1.f;
	float PendingEndShakeScale = 0.5f;

	// the player whose CustomTimeDilation counter-boost needs resetting when the slow-mo ends
	TWeakObjectPtr<APawn> SlowMoPlayer;

	// replayed on the player's camera when the slow-mo ends (the "snap back" half of the shake pair)
	TSubclassOf<UCameraShakeBase> PendingEndShakeClass;
};
