// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_CoolDown.generated.h"

/**
 * 
 */
/**
 * Generic cooldown effect. Has a duration but the seconds are injected per-cast via
 * SetByCaller (Data.CooldownDuration), and the ability adds its own cooldown tag dynamically -
 * so one class covers skill + ultimate with independent cooldowns.
 */
UCLASS()
class WUTHERINGWAVES_API UGE_CoolDown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_CoolDown();
};
