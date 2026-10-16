// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameAbilities/GA_ResonanceSkill.h"
#include "TimerManager.h"
#include "GA_Liberation.generated.h"

class ACameraActor;
class APlayerController;
class ULiberationCutinWidget;
class AWuwaHUD;

/**
 * Ultimate (Resonance Liberation).
 *
 * The cast flow is identical to a resonance skill (inherited from UGA_ResonanceSkill): it plays the
 * montage mapped to its own asset tag (Ability.Type.Liberation), soft-locks the nearest enemy, and
 * applies that entry's damage / groggy / variation on the weapon-notify hit event.
 *
 * Ultimate-specific rule added here: it can only be cast when the ultimate gauge (UltimateEnergy) is
 * FULL, and casting drains it back to 0.
 */
UCLASS()
class WUTHERINGWAVES_API UGA_Liberation : public UGA_ResonanceSkill
{
	GENERATED_BODY()

public:
	UGA_Liberation();

	// gate: only allow activation while the ultimate gauge is full
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// consume the gauge, then run the shared skill flow
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Liberation|Cutin")
	bool bPlayCutin = true;

	UPROPERTY(EditDefaultsOnly, Category = "Liberation|Cutin", meta = (ClampMin = "0.2", ClampMax = "5.0"))
	float CutinDuration = 1.6f;

private:
	void StartCutin();
	void FinishCutin();
	void CleanupCutin();

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> CutinCamera;

	UPROPERTY(Transient)
	TObjectPtr<ULiberationCutinWidget> CutinWidget;

	TWeakObjectPtr<APlayerController> CutinPlayerController;
	TWeakObjectPtr<AWuwaHUD> HiddenHud;
	TWeakObjectPtr<AActor> PreviousViewTarget;
	FTimerHandle CutinTimer;
};
