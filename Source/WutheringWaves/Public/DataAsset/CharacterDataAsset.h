// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "CharacterDataAsset.generated.h"


USTRUCT(BlueprintType)
struct FSkillData
{
	GENERATED_BODY()

	// HUD icon for this ability shown on the skill bar. Accepts a Texture2D or a Paper2D Sprite.
	UPROPERTY(EditDefaultsOnly, meta = (AllowedClasses = "/Script/Engine.Texture2D, /Script/Paper2D.PaperSprite"))
	TObjectPtr<UObject> Icon;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> Montage;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UGameplayEffect> DamageEffect;
	
	// flat damage this skill deals. Injected into the damage GE via SetByCaller (Data.Damage) - same as enemies.
	UPROPERTY(EditDefaultsOnly)
	float Damage = 10.f;

	// how much this skill drains from an enemy's groggy gauge on hit
	UPROPERTY(EditDefaultsOnly)
	float GroggyDamage = 0.f;

	// 변주 게이지 (charged-swap circuit) the attacker gains when this skill hits
	UPROPERTY(EditDefaultsOnly)
	float VariationGain = 0.f;

	// 궁극기 효율 게이지 the attacker gains when this skill hits (fills toward the ultimate/liberation)
	UPROPERTY(EditDefaultsOnly)
	float UltimateGain = 0.f;

	// --- support buff (수수 등) : used by GA_TeamBuff, ignored by damage skills ---
	// how much this skill adds to every teammate's AttackPower (0.3 = +30% damage while active)
	UPROPERTY(EditDefaultsOnly)
	float BuffAmount = 0.f;

	// how long the attack buff lasts, in seconds
	UPROPERTY(EditDefaultsOnly)
	float BuffDuration = 0.f;

	// voice lines for this skill; one is picked at random and played when the skill is used
	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<class USoundBase>> VoiceLines;

	UPROPERTY(EditDefaultsOnly)
	float Cooldown = 0.f;


};

// Auto-target ("soft lock") tuning. On attack / skill / ultimate the character turns toward
// the best nearby enemy and optionally slides in ("follows" it). Kept per-character so the
// feel stays data-driven (a big melee wants a bigger step-in, a ranged char rotates only).
USTRUCT(BlueprintType)
struct FTargetAssistData
{
	GENERATED_BODY()

	// master switch - turn the whole assist off for a character if desired
	UPROPERTY(EditDefaultsOnly)
	bool bEnabled = true;

	// enemies farther than this (from the character) are ignored
	UPROPERTY(EditDefaultsOnly)
	float Range = 800.f;

	// only enemies within this half-angle (deg) of the camera's forward are eligible, so we lock
	// onto what the player is roughly aiming at rather than something off to the side / behind
	UPROPERTY(EditDefaultsOnly)
	float ConeHalfAngleDeg = 60.f;

	// slide toward the target on activation (the "follow" feel). false = rotate only (e.g. ranged)
	UPROPERTY(EditDefaultsOnly)
	bool bStepIn = true;

	// stop this far short of the target so we don't overlap it
	UPROPERTY(EditDefaultsOnly)
	float StopDistance = 150.f;

	// never slide more than this in one assist (keeps a far target from yanking us across the arena)
	UPROPERTY(EditDefaultsOnly)
	float MaxStepDistance = 400.f;

	// seconds the turn / step-in blend runs
	UPROPERTY(EditDefaultsOnly)
	float BlendTime = 0.18f;
};

// Dodge has one input but three montages picked by situation, so it gets its own
// block instead of the Skills map (which is one-montage-per-tag and carries damage data).
USTRUCT(BlueprintType)
struct FDodgeData
{
	GENERATED_BODY()

	// Played when an enemy is attacking nearby (perfect dodge).
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> PerfectMontage;

	// Played when a movement key is held: dash in the input direction.
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> ForwardMontage;

	// Played with no movement input: dash backward.
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> BackMontage;

	// whoosh played on any dodge
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class USoundBase> DodgeSound;

	// distinct sound for a perfect dodge (falls back to DodgeSound if left empty)
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class USoundBase> PerfectDodgeSound;
};

UCLASS()
class WUTHERINGWAVES_API UCharacterDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Character.Element"))
	FGameplayTag ElementTag;
	
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Ability.Type"))
	TMap<FGameplayTag, FSkillData> Skills;
	
	UPROPERTY(EditDefaultsOnly, meta = (Categories = "Character.Range"))
	FGameplayTag RangeTag;

	// portrait shown in the right-side team panel
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UTexture2D> Portrait;

	// per-character 공명 회로 image shown behind/around the resonance energy bar. Texture2D or Paper2D Sprite.
	UPROPERTY(EditDefaultsOnly, meta = (AllowedClasses = "/Script/Engine.Texture2D, /Script/Paper2D.PaperSprite"))
	TObjectPtr<UObject> ResonanceCircuitIcon;

	// played by GA_Intro when this character is swapped in via a charged swap (flank appear)
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> IntroMontage;

	// voice line played when this character is swapped in. Any swap voice still playing is cut first (no overlap).
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class USoundBase> SwapVoice;

	// flinch / hit-react montage played when this character takes damage but survives
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<class UAnimMontage> HitReactMontage;

	// hit voice lines - one is played at random when this character takes damage (empty = silent)
	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<class USoundBase>> HitVoiceLines;

	// The three dodge montages for this character (perfect / forward / back).
	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	FDodgeData Dodge;

	// auto-target ("soft lock") behaviour when this character attacks
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	FTargetAssistData TargetAssist;

	
	// max 변주 게이지 (charged-swap circuit). The gauge fills to this on hits.
	UPROPERTY(EditDefaultsOnly)
	float MaxVariationEnergy = 100.f;

	// movement speeds - hold the dodge key to run (UE default walk is ~600 for reference)
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RunSpeed = 100.f;

	// To give different hp per character -> Later will use functions to give that
	UPROPERTY(EditDefaultsOnly)
	float MaxHp = 100.f;
	
	UPROPERTY(EditDefaultsOnly)
	int32 Level = 1;
	
};
