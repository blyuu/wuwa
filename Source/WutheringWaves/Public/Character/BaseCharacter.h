#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "AbilitySystemInterface.h"
#include "BaseCharacter.generated.h"

class UGameplayAbility;
struct FGameplayTag;

UCLASS()
class WUTHERINGWAVES_API ABaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	//Virtual Function
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	
protected:
	
	//GAS System Interface
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	//Game Abilities List
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem")
	TArray<TSubclassOf<UGameplayAbility>> Abilites;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UCameraComponent> CameraComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class USpringArmComponent> SpringArmComponent;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<class UWuwaInputConfig> WuwaInputConfig;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<class UInputMappingContext> InputMappingContext;
	
	UFUNCTION()
	void InputTagUseAbility(FGameplayTag InputTag);
	
	void GiveAbilites();
	
	virtual void PossessedBy(AController* NewController) override;

	
public:
	
	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);
	void MouseWheel(const FInputActionValue& value);
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UAnimMontage> BaseAttack;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<class UWuWa_AttributeSetBase> AttributeSet;
	
};
