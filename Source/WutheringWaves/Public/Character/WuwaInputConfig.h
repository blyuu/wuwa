#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "InputTriggers.h"
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

	// Which trigger event fires the ability. Completed = on release (attacks),
	// Triggered = when a Tap/Hold trigger fires (dodge on a Tap trigger), Started = on press.
	UPROPERTY(EditAnywhere)
	ETriggerEvent TriggerEvent = ETriggerEvent::Completed;

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

	// Sprint (run): map to the same key as dodge with a HOLD trigger (tap = dodge, hold = run)
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<UInputAction> InputSprint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FWuwaInput> InputTagList;
	
};
