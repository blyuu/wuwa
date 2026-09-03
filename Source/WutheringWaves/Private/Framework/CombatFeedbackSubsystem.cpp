// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CombatFeedbackSubsystem.h"

void UCombatFeedbackSubsystem::ReportDamage(const FCombatFeedbackEvent& Event)
{
	OnDamage.Broadcast(Event);
}
