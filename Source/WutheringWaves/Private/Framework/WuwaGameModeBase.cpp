#include "Framework/WuwaGameModeBase.h"

#include "Character/WuwaPlayerController.h"


AWuwaGameModeBase::AWuwaGameModeBase()
{
	// 팀 시스템(TeamComponent)이 폰을 직접 스폰/possess 하므로
	// GameMode의 자동 폰 스폰은 끈다 (안 그러면 폰이 이중으로 생김)
	DefaultPawnClass = nullptr;

	PlayerControllerClass = AWuwaPlayerController::StaticClass();
}
