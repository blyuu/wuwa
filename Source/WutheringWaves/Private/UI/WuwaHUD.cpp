// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WuwaHUD.h"

#include "UI/WuwaOverlayWidget.h"
#include "UI/WuwaBossHealthWidget.h"
#include "UI/TeamPortraitWidget.h"
#include "UI/DamageNumberWidget.h"
#include "Character/PlayableCharacter.h"
#include "Character/TeamComponent.h"
#include "Enemy/EnemyCharacter.h"
#include "Framework/CombatFeedbackSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void AWuwaHUD::BeginPlay()
{
	Super::BeginPlay();

	EnsureOverlay();

	// Cover the case where the HUD is spawned *after* the pawn is already possessed:
	// seed the overlay from whoever we're currently controlling.
	if (APlayerController* PC = GetOwningPlayerController())
	{
		if (APlayableCharacter* Char = Cast<APlayableCharacter>(PC->GetPawn()))
		{
			OnPlayerCharacterChanged(Char);
		}
	}

	// listen for damage so we can pop floating numbers over enemies
	if (UWorld* World = GetWorld())
	{
		if (UCombatFeedbackSubsystem* Feedback = World->GetSubsystem<UCombatFeedbackSubsystem>())
		{
			DamageDelegateHandle = Feedback->OnDamage.AddUObject(this, &AWuwaHUD::HandleDamageEvent);
		}
	}
}

void AWuwaHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UCombatFeedbackSubsystem* Feedback = World->GetSubsystem<UCombatFeedbackSubsystem>())
		{
			Feedback->OnDamage.Remove(DamageDelegateHandle);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AWuwaHUD::HandleDamageEvent(const FCombatFeedbackEvent& Event)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !DamageNumberClass)
	{
		return;
	}

	// world hit position -> screen. Behind the camera -> skip (don't draw a number off-screen).
	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(Event.WorldLocation, ScreenPos))
	{
		return;
	}

	// small scatter so stacked hits fan out instead of overlapping perfectly (fountain feel)
	ScreenPos.X += FMath::FRandRange(-25.f, 25.f);
	ScreenPos.Y += FMath::FRandRange(-15.f, 5.f);

	UDamageNumberWidget* Number = CreateWidget<UDamageNumberWidget>(PC, DamageNumberClass);
	if (!Number)
	{
		return;
	}

	Number->SetDamage(Event.Amount, Event.ElementTag);
	Number->AddToViewport();
	// true = convert the pixel position into DPI-scaled viewport space so it lands on the enemy
	Number->SetPositionInViewport(ScreenPos, true);
}

void AWuwaHUD::EnsureOverlay()
{
	if (OverlayWidget || !OverlayClass)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	OverlayWidget = CreateWidget<UWuwaOverlayWidget>(PC, OverlayClass);
	if (OverlayWidget)
	{
		OverlayWidget->AddToViewport();
	}
}

void AWuwaHUD::EnsureTeamPortrait()
{
	if (TeamPortraitWidget || !TeamPortraitClass)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	TeamPortraitWidget = CreateWidget<UTeamPortraitWidget>(PC, TeamPortraitClass);
	if (TeamPortraitWidget)
	{
		TeamPortraitWidget->AddToViewport();
	}
}

void AWuwaHUD::OnPlayerCharacterChanged(APlayableCharacter* NewCharacter)
{
	if (!NewCharacter)
	{
		return;
	}

	// The team system may notify us before BeginPlay ran, so make sure the widgets exist.
	EnsureOverlay();
	if (OverlayWidget)
	{
		OverlayWidget->SetAbilitySystemComponent(NewCharacter->GetAbilitySystemComponent());
		OverlayWidget->SetLevel(NewCharacter->GetCharacterLevel());
		OverlayWidget->SetSkillIcons(NewCharacter->GetCharacterData());
	}

	// team portrait panel: refresh the 3 portraits + point the concerto gauge at the active character
	EnsureTeamPortrait();
	if (TeamPortraitWidget)
	{
		if (APlayerController* PC = GetOwningPlayerController())
		{
			if (UTeamComponent* Team = PC->FindComponentByClass<UTeamComponent>())
			{
				TeamPortraitWidget->SetTeam(Team->GetTeam(), Team->GetActiveIndex());
			}
		}
		TeamPortraitWidget->SetAbilitySystemComponent(NewCharacter->GetAbilitySystemComponent());
	}
}

void AWuwaHUD::ShowBossBar(AEnemyCharacter* Boss)
{
	if (!Boss || !BossHealthClass)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	// Create the bar once, reuse it if another boss appears.
	if (!BossWidget)
	{
		BossWidget = CreateWidget<UWuwaBossHealthWidget>(PC, BossHealthClass);
		if (!BossWidget)
		{
			return;
		}
		BossWidget->AddToViewport();
	}

	BossWidget->SetAbilitySystemComponent(Boss->GetAbilitySystemComponent());
	BossWidget->SetBossInfo(Boss->EnemyDataAsset);
}

void AWuwaHUD::HideBossBar()
{
	if (BossWidget)
	{
		BossWidget->RemoveFromParent();
		BossWidget = nullptr;
	}
}
