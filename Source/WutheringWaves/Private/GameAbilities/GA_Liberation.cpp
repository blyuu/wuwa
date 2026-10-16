// Fill out your copyright notice in the Description page of Project Settings.


#include "GameAbilities/GA_Liberation.h"

#include "AbilitySystemComponent.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "GameplayTags/WuwaGameplayTags.h"
#include "Camera/CameraActor.h"
#include "Framework/LiberationCameraActor.h"
#include "Camera/CameraComponent.h"
#include "Character/PlayableCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/LiberationCutinWidget.h"
#include "UI/WuwaHUD.h"
#include "Blueprint/UserWidget.h"

namespace
{
	FLinearColor GetLiberationAccent(const FGameplayTag& Element)
	{
		if (Element == ElementTags::Character_Element_Fusion) return FLinearColor(1.f, .32f, .08f);
		if (Element == ElementTags::Character_Element_Glacio) return FLinearColor(.25f, .78f, 1.f);
		if (Element == ElementTags::Character_Element_Aero) return FLinearColor(.16f, .92f, .65f);
		if (Element == ElementTags::Character_Element_Electro) return FLinearColor(.66f, .32f, 1.f);
		if (Element == ElementTags::Character_Element_Spectro) return FLinearColor(1.f, .82f, .27f);
		if (Element == ElementTags::Character_Element_Havoc) return FLinearColor(.85f, .18f, .5f);
		return FLinearColor(.25f, .72f, 1.f);
	}
}

UGA_Liberation::UGA_Liberation()
{
	// the whole ultimate is invulnerable (cinematic cast): while active the owner has State.Invulnerable,
	// so incoming damage is rejected and the hit-react can't interrupt the montage. GAS removes it on end.
	ActivationOwnedTags.AddTag(StateTags::State_Invulnerable);
}

bool UGA_Liberation::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return false;
	}

	// gauge must be full to fire the ultimate
	const float Energy = ASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetUltimateEnergyAttribute());
	const float MaxEnergy = ASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxUltimateEnergyAttribute());
	return MaxEnergy > 0.f && Energy >= MaxEnergy - 0.01f;
}

void UGA_Liberation::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// drain the ultimate gauge to 0 on cast (CanActivateAbility already guaranteed it was full)
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ApplyModToAttribute(UWuWa_AttributeSetBase::GetUltimateEnergyAttribute(), EGameplayModOp::Override, 0.f);
	}

	// run the shared resonance-skill flow (montage + soft-lock + hit-damage)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (bPlayCutin && IsActive())
	{
		StartCutin();
	}
}

void UGA_Liberation::StartCutin()
{
	APlayableCharacter* Character = Cast<APlayableCharacter>(GetAvatarActorFromActorInfo());
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	UWorld* World = GetWorld();
	if (!Character || !PC || !PC->IsLocalController() || !World) return;

	CleanupCutin();
	CutinPlayerController = PC;
	PreviousViewTarget = PC->GetViewTarget();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PC;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALiberationCameraActor* PortraitCamera = World->SpawnActor<ALiberationCameraActor>(
		ALiberationCameraActor::StaticClass(), Character->GetActorLocation(), FRotator::ZeroRotator, SpawnParameters);
	if (!PortraitCamera) return;
	CutinCamera = PortraitCamera;
	PortraitCamera->Initialize(Character);
	CutinWidget = CreateWidget<ULiberationCutinWidget>(PC, ULiberationCutinWidget::StaticClass());
	if (CutinWidget)
	{
		CutinWidget->StartCutin(CutinDuration,
			GetLiberationAccent(Character->CharacterData ? Character->CharacterData->ElementTag : FGameplayTag()));
		CutinWidget->AddToViewport(100);
	}
	if (AWuwaHUD* HUD = Cast<AWuwaHUD>(PC->GetHUD()))
	{
		HiddenHud = HUD;
		HUD->SetCinematicHudHidden(true);
	}
	PC->SetViewTarget(CutinCamera);
	World->GetTimerManager().SetTimer(CutinTimer, this, &UGA_Liberation::FinishCutin, CutinDuration, false);
}

void UGA_Liberation::FinishCutin()
{
	// End the graphic beat and reveal the HUD, but keep the front view for the rest
	// of the ultimate montage. EndAbility restores the normal gameplay camera.
	if (CutinWidget)
	{
		CutinWidget->RemoveFromParent();
		CutinWidget = nullptr;
	}
	if (AWuwaHUD* HUD = HiddenHud.Get())
	{
		HUD->SetCinematicHudHidden(false);
	}
	HiddenHud.Reset();
}

void UGA_Liberation::CleanupCutin()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CutinTimer);
	}
	if (CutinWidget)
	{
		CutinWidget->RemoveFromParent();
		CutinWidget = nullptr;
	}
	if (AWuwaHUD* HUD = HiddenHud.Get())
	{
		HUD->SetCinematicHudHidden(false);
	}
	HiddenHud.Reset();
	if (APlayerController* PC = CutinPlayerController.Get())
	{
		if (PC->GetViewTarget() == CutinCamera)
		{
			if (AActor* Target = PreviousViewTarget.Get())
			{
				PC->SetViewTarget(Target);
			}
			else if (APawn* Pawn = PC->GetPawn())
			{
				PC->SetViewTarget(Pawn);
			}
		}
	}
	if (CutinCamera)
	{
		CutinCamera->Destroy();
		CutinCamera = nullptr;
	}
	CutinPlayerController.Reset();
	PreviousViewTarget.Reset();
}

void UGA_Liberation::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	CleanupCutin();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
