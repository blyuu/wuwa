#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "BaseCharacter.generated.h"

class UGameplayAbility;

UCLASS()
class WUTHERINGWAVES_API ABaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	//=====================================================================================
	// GAS Core
	// 1. ASC
	// 2. AttributeSet
	// 3. Abilities + GiveAbilites
	//=====================================================================================

	//GAS System Interface
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<class UWuWa_AttributeSetBase> AttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem")
	TArray<TSubclassOf<UGameplayAbility>> Abilities;

	void GiveAbilites();

	// Ability Should be only Granted 1 time so to prevent that added  this
	bool bAbilitiesGranted = false;
	
	
	//Helper Function of HP and XP etc
	void InitializeAttributes(float InMaxHp);
	
	bool bIsDead = false;

public:
	//=======================================================================================
	//  Not Sure Yet if i am gonna keep this thing here
	//=======================================================================================
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class AWeaponClass> DefaultWeaponClass;

	// whether to keep the weapon always visible (for characters with no draw/sheathe like enemies)
	// false = normal behavior, hidden by default and drawn only during the attack ability
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool bAlwaysShowWeapon = false;

	virtual void HandleDeath();
	// Actual Weapon that the player held
	UPROPERTY()
	AWeaponClass* CurrentWeapon;
};
