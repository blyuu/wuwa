// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WuwaHUD.h"

#include "UI/WuwaOverlayWidget.h"
#include "Character/PlayableCharacter.h"

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

void AWuwaHUD::OnPlayerCharacterChanged(APlayableCharacter* NewCharacter)
{
	// The team system may notify us before BeginPlay ran, so make sure the widget exists.
	EnsureOverlay();

	if (!OverlayWidget || !NewCharacter)
	{
		return;
	}

	OverlayWidget->SetAbilitySystemComponent(NewCharacter->GetAbilitySystemComponent());
	OverlayWidget->SetLevel(NewCharacter->GetCharacterLevel());
	OverlayWidget->SetSkillIcons(NewCharacter->GetCharacterData());
}
