// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify/AN_TrackTarget.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

void UAN_TrackTarget::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	// the current player character is whoever is possessed (the active team member)
	APawn* Player = UGameplayStatics::GetPlayerPawn(MeshComp, 0);
	if (!Player)
	{
		return;
	}

	FVector ToPlayer = Player->GetActorLocation() - Owner->GetActorLocation();
	ToPlayer.Z = 0.f;
	if (ToPlayer.IsNearlyZero())
	{
		return;
	}

	const FRotator CurrentRot = Owner->GetActorRotation();
	FRotator GoalRot = ToPlayer.Rotation();
	GoalRot.Pitch = CurrentRot.Pitch;   // yaw only - keep the enemy upright
	GoalRot.Roll  = CurrentRot.Roll;

	// already facing closely enough -> don't jitter
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, GoalRot.Yaw)) <= AngleDeadzoneDeg)
	{
		return;
	}

	const FRotator NewRot = (TurnSpeed > 0.f)
		? FMath::RInterpTo(CurrentRot, GoalRot, FrameDeltaTime, TurnSpeed)
		: GoalRot;

	Owner->SetActorRotation(NewRot);
}
