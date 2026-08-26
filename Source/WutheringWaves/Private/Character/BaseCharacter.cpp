#include "Character/BaseCharacter.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/WeaponClass.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"


ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UWuWa_AttributeSetBase>(TEXT("AttributeSystem"));
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (DefaultWeaponClass)
	{
		CurrentWeapon = GetWorld()->SpawnActor<AWeaponClass>(DefaultWeaponClass);
		CurrentWeapon->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		TEXT("WeaponProp02"));
		CurrentWeapon->HideWeapon();
	}
}


void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	
	GiveAbilites();
}

void ABaseCharacter::GiveAbilites()
{
	// At Character changed Possessed is called the, ability is only once
	if (bAbilitiesGranted)
	{
		return;
	}

	if (Abilities.IsEmpty())
	{
		return;
	}

	bAbilitiesGranted = true;

	//GameAbilities that are set at the BP are all granted
	for (int i = 0; i<Abilities.Num();i++)
	{
		FGameplayAbilitySpec AbilitySpec(Abilities[i], 1, -1);
		FGameplayAbilitySpecHandle Checker = AbilitySystemComponent->GiveAbility(AbilitySpec);
	}
}

void ABaseCharacter::InitializeAttributes(float InMaxHp)
{
	if (!AttributeSet)
	{
		return;
	}
	
	AttributeSet->InitMaxHp(InMaxHp);
	AttributeSet->InitHp(InMaxHp);
}

void ABaseCharacter::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}
	
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	
	SetActorEnableCollision(false);
	
	UE_LOG(LogTemp,Warning, TEXT("%s Dead"), *GetName());
	
}
