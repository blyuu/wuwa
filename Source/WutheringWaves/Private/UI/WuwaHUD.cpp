// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WuwaHUD.h"

#include "UI/WuwaOverlayWidget.h"
#include "UI/WuwaBossHealthWidget.h"
#include "UI/TeamPortraitWidget.h"
#include "Character/PlayableCharacter.h"
#include "Character/TeamComponent.h"
#include "Enemy/EnemyCharacter.h"

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
