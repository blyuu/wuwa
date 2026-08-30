#include "Framework/WuwaGameModeBase.h"

#include "Character/WuwaPlayerController.h"


AWuwaGameModeBase::AWuwaGameModeBase()
{
	// the team system (TeamComponent) spawns/possesses pawns itself
	// so turn off GameMode's auto pawn spawn (otherwise pawns get created twice)
	DefaultPawnClass = nullptr;

	PlayerControllerClass = AWuwaPlayerController::StaticClass();
}
