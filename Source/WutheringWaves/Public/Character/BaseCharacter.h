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

	// uniform scale applied to the spawned weapon. Bump it for big enemies so the weapon reads bigger;
	// the hit trace grows with it too, because the WeaponRoot/WeaponTip sockets scale with the mesh. 1 = default.
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float WeaponScale = 1.f;

	virtual void HandleDeath();

	// true once HandleDeath has run - target-assist / AI use this to skip corpses
	bool IsDead() const { return bIsDead; }

	// Actual Weapon that the player held
	UPROPERTY()
	AWeaponClass* CurrentWeapon;
};
