// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/WeaponAnimNotifyState.h"
#include "Character/BaseCharacter.h"
#include "Character/PlayableCharacter.h"
#include "Character/WeaponClass.h"
#include "Enemy/EnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

#if ENABLE_DRAW_DEBUG
// Capture toggle for screenshots: console `wuwa.DebugFallback 1` to draw the fallback-correction gate.
// Off in normal play; the draw calls compile out entirely in Shipping (ENABLE_DRAW_DEBUG == 0).
static TAutoConsoleVariable<int32> CVarDebugFallback(
	TEXT("wuwa.DebugFallback"),
	0,
	TEXT("Draw the melee fallback-correction gate: front cone (yellow) + range ring, and the ")
	TEXT("line to the candidate (green = correction fired, red = rejected by the gate). 0=off, 1=on."),
	ECVF_Cheat);
#endif

void UWeaponAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);

	HitActors.Empty();

	ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner());
	if (!Character || !Character->CurrentWeapon) return;

	USkeletalMeshComponent* WeaponMesh = Character->CurrentWeapon->SkeletalMeshComponent;
	PrevSocketRoot = WeaponMesh->GetSocketLocation(AWeaponClass::SocketWeaponRoot);
	PrevSocketTip  = WeaponMesh->GetSocketLocation(AWeaponClass::SocketWeaponTip);
}

void UWeaponAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);

	ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner());
	if (!Character || !Character->CurrentWeapon) return;

	USkeletalMeshComponent* WeaponMesh = Character->CurrentWeapon->SkeletalMeshComponent;
	const FVector CurrSocketRoot = WeaponMesh->GetSocketLocation(AWeaponClass::SocketWeaponRoot);
	const FVector CurrSocketTip  = WeaponMesh->GetSocketLocation(AWeaponClass::SocketWeaponTip);

	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore = { Character };

	// Two sweeps, hits merged:
	//  1) tip arc (prev tip -> curr tip): covers the space the tip crossed BETWEEN frames (fast swings)
	//  2) whole blade this frame (root -> tip): catches enemies standing CLOSER than the tip, which a
	//     tip-only trace would arc right over. This is the main reason hits felt like they whiffed.
	auto SweepInto = [&](const FVector& Start, const FVector& End)
	{
		TArray<FHitResult> Hits;
		UKismetSystemLibrary::SphereTraceMulti(
			MeshComp,
			Start,
			End,
			TraceRadius,
			UEngineTypes::ConvertToTraceType(ECC_Pawn),
			false,
			ActorsToIgnore,
			EDrawDebugTrace::None,
			Hits,
			true
		);
		HitResults.Append(Hits);
	};

	SweepInto(PrevSocketTip, CurrSocketTip);   // tip arc between frames
	SweepInto(CurrSocketRoot, CurrSocketTip);  // full blade this frame

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActors.Contains(HitActor)) continue;

		HitActors.Add(HitActor);

		UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
		if (!ASC) continue;

		FGameplayEventData EventData;
		FGameplayAbilityTargetData_SingleTargetHit* TargetData = new FGameplayAbilityTargetData_SingleTargetHit(Hit);
		EventData.TargetData.Add(TargetData);
		EventData.Instigator = Character;
		EventData.Target     = HitActor;

		// fire with the tag set on the montage if not set fall back to the player attack tag
		const FGameplayTag EventToSend = HitEventTag.IsValid() ? HitEventTag : EventTags::Event_Attack_Hit;
		ASC->HandleGameplayEvent(EventToSend, &EventData);
	}

	PrevSocketRoot = CurrSocketRoot;
	PrevSocketTip  = CurrSocketTip;
}

void UWeaponAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::NotifyEnd(MeshComp, Animation);

	// game-feel correction: if the whole swing's precise sweep hit nobody, still register a hit on the
	// target the attacker is clearly facing (within FallbackRange + a front cone).
	if (bUseFallbackHit && HitActors.Num() == 0)
	{
		TryFallbackHit(MeshComp);
	}

	HitActors.Empty();
	PrevSocketRoot = FVector::ZeroVector;
	PrevSocketTip  = FVector::ZeroVector;
}

void UWeaponAnimNotifyState::TryFallbackHit(USkeletalMeshComponent* MeshComp)
{
	ABaseCharacter* Character = MeshComp ? Cast<ABaseCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!Character)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// pick the fallback target: the player aims at the enemy it's facing; an enemy aims at the player
	AActor* Target = nullptr;
	if (APlayableCharacter* Player = Cast<APlayableCharacter>(Character))
	{
		Target = Player->AcquireTargetEnemy();
	}
	else
	{
		Target = UGameplayStatics::GetPlayerPawn(Character, 0);
	}
	if (!Target)
	{
		return;
	}

	// don't correct onto a corpse
	if (ABaseCharacter* TargetChar = Cast<ABaseCharacter>(Target))
	{
		if (TargetChar->IsDead())
		{
			return;
		}
	}

#if ENABLE_DRAW_DEBUG
	// Draws the exact gate the correction uses (front cone + range ring) and a line to the candidate.
	// Called at the point of decision so screenshots show WHY the swing did/didn't get corrected.
	const bool bDbgFallback = CVarDebugFallback.GetValueOnGameThread() > 0;
	auto DrawFallbackGate = [&](const FColor& OutcomeColor)
	{
		UWorld* World = Character->GetWorld();
		if (!bDbgFallback || !World) return;

		const FVector Origin = Character->GetActorLocation();
		FVector Fwd = Character->GetActorForwardVector();
		Fwd.Z = 0.f; Fwd.Normalize();

		const float ConeRad = FMath::DegreesToRadians(FallbackConeHalfAngleDeg);
		const float Life = 2.5f;   // stays alive long enough to Pause + HighResShot

		DrawDebugCone(World, Origin, Fwd, FallbackRange, ConeRad, ConeRad, 24, FColor(255, 200, 0), false, Life, 0, 1.5f);
		DrawDebugCircle(World, Origin, FallbackRange, 48, FColor(120, 120, 120), false, Life, 0, 1.f, FVector(1, 0, 0), FVector(0, 1, 0), false);

		// green = correction actually fired, red = rejected by range/cone
		DrawDebugLine(World, Origin, Target->GetActorLocation(), OutcomeColor, false, Life, 0, 3.f);
		DrawDebugSphere(World, Target->GetActorLocation(), 40.f, 12, OutcomeColor, false, Life);
	};
#endif

	// range + front-cone gate so we only "correct" onto something the swing plausibly aimed at
	FVector ToTarget = Target->GetActorLocation() - Character->GetActorLocation();
	ToTarget.Z = 0.f;
	const float Dist = ToTarget.Size();
	if (Dist > FallbackRange || Dist < KINDA_SMALL_NUMBER)
	{
#if ENABLE_DRAW_DEBUG
		DrawFallbackGate(FColor::Red);
#endif
		return;
	}
	ToTarget /= Dist;

	FVector Forward = Character->GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(FallbackConeHalfAngleDeg));
	if (FVector::DotProduct(Forward, ToTarget) < CosHalf)
	{
#if ENABLE_DRAW_DEBUG
		DrawFallbackGate(FColor::Red);
#endif
		return;
	}

	// fire the same hit event a real sweep would - OnHitEvent applies damage (and the enemy can still dodge)
	FGameplayEventData EventData;
	EventData.Instigator = Character;
	EventData.Target     = Target;

	const FGameplayTag EventToSend = HitEventTag.IsValid() ? HitEventTag : EventTags::Event_Attack_Hit;
	ASC->HandleGameplayEvent(EventToSend, &EventData);

#if ENABLE_DRAW_DEBUG
	DrawFallbackGate(FColor::Green);
#endif
}
