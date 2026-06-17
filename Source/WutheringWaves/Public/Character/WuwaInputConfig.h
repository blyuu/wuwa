#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WuwaInputConfig.generated.h"


class UInputAction;

UCLASS()
class WUTHERINGWAVES_API UWuwaInputConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<UInputAction> InputMove;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<UInputAction> InputLook;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<UInputAction> InputJump;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<UInputAction> InputMouseWheel;
};
