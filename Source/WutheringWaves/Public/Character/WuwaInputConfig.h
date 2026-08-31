#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "WuwaInputConfig.generated.h"


class UInputAction;
struct FGameplayTag;

USTRUCT(Blueprintable)
struct FWuwaInput
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UInputAction> InputAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag InputTag;

	// Fire on press (Started) instead of on release (Completed).
	// Turn ON for dodge so it reacts the instant you click; leave OFF to keep the old release-to-fire feel.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTriggerOnPressed = false;

};

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FWuwaInput> InputTagList;
	
};
