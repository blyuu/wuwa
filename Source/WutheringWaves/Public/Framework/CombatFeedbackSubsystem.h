// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
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
 * Central hub that decouples gameplay from combat UI. The AttributeSet reports every damage instance
 * here (ReportDamage); the HUD subscribes to OnDamage and spawns floating numbers. One world = one hub.
 */
UCLASS()
class WUTHERINGWAVES_API UCombatFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// gameplay reports a damage instance; broadcasts to every listener
	void ReportDamage(const FCombatFeedbackEvent& Event);

	// the HUD binds to this to pop screen-space damage numbers
	FOnCombatDamage OnDamage;
};
