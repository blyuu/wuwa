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
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	//=====================================================================================
	// Default Components 
	//=====================================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UCameraComponent> CameraComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class USpringArmComponent> SpringArmComponent;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<class UInputMappingContext> InputMappingContext;
	
	
	//=====================================================================================
	// GAS 
	// 1. ASC
	// 2. Abilities 
	// 3. InputConfig
	// 4. InputTagUseAbility
	// 5. GiveAbilites
	//=====================================================================================
	
	//GAS System Interface
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<class UWuWa_AttributeSetBase> AttributeSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem")
	TArray<TSubclassOf<UGameplayAbility>> Abilities;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TObjectPtr<class UWuwaInputConfig> WuwaInputConfig;
	
	UFUNCTION()
	void InputTagUseAbility(FGameplayTag InputTag);
	
	void GiveAbilites();

	// 어빌리티는 캐릭터당 1회만 부여되어야 함 (교체 시 possess가 반복 호출되므로 중복 방지)
	bool bAbilitiesGranted = false;

	// IMC 추가/제거 헬퍼 (활성 캐릭터만 입력 매핑을 갖도록)
	void AddInputMapping();
	void RemoveInputMapping();


public:
	//========================================================================
	// Character Movements Move,Look, Zoom (캐릭터 이동 관련 함수 : 움직임, 시야)
	//========================================================================
	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);
	void MouseWheel(const FInputActionValue& value);
	
	
	//=======================================================================================
	//  Character Data like Element,AnimMontage etc (캐릭터 데이터를 에디터에서 할당: 속성 및 몽타주) 
	//=======================================================================================
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UCharacterDataAsset> CharacterData;
	
	
	//=======================================================================================
	//  Weapon Class to be Chosen at Editor, the Weapon Class is derived from the Weapon Data Asset
	//  장착될 무기 정보를 가지는 객체
	//=======================================================================================
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class AWeaponClass> DefaultWeaponClass;
	
	//Weapon Actor that is really attached to the Character (실제 장착되는 무기 객체)
	UPROPERTY()
	AWeaponClass* CurrentWeapon;
	
	
};
