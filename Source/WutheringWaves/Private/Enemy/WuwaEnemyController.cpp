// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/WuwaEnemyController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BrainComponent.h"
#include "Enemy/EnemyCharacter.h"

void AWuwaEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// the BT reference lives on the enemy (BP_Enemy) -> read it from the possessed pawn and run it
	// (each enemy type can have a different BT and the controller doesn't need to know the type)
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(InPawn);
	if (Enemy && Enemy->BehaviorTree)
	{
		// RunBehaviorTree also auto inits the Blackboard component from the Blackboard asset set on the BT
		RunBehaviorTree(Enemy->BehaviorTree);

		// if we possessed AFTER the enemy started its spawn intro, hold combat until the intro finishes
		// (the enemy resumes us from OnIntroMontageEnded). For placed enemies the enemy pauses us directly.
		if (Enemy->IsPlayingIntro() && GetBrainComponent())
		{
			GetBrainComponent()->PauseLogic(TEXT("Intro"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyController] No BehaviorTree set on %s"),
			InPawn ? *InPawn->GetName() : TEXT("null"));
	}
}
