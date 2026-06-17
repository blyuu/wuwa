#include "Framework/WuwaGameModeBase.h"

#include "Character/BaseCharacter.h"


AWuwaGameModeBase::AWuwaGameModeBase()
{
	DefaultPawnClass = ABaseCharacter::StaticClass();
}
