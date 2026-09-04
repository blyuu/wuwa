#include "Character/BaseCharacter.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/WeaponClass.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"


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

		// attach ignores scale, so set the weapon's own scale here (per-character, e.g. bigger for enemies)
		CurrentWeapon->SetActorScale3D(FVector(WeaponScale));

		// always visible characters (enemies) keep it drawn, otherwise hide it and draw only on attack
		if (bAlwaysShowWeapon)
		{
			CurrentWeapon->ShowWeapon();
		}
		else
		{
			CurrentWeapon->HideWeapon();
		}
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

UAnimMontage* ABaseCharacter::GetHitReactMontage() const
{
	// base: the BP-set fallback (enemies). Playable characters override this to read their data asset.
	return HitReactMontage;
}

const TArray<TObjectPtr<USoundBase>>& ABaseCharacter::GetHitVoiceLines() const
{
	// base: the BP-set fallback (enemies). Playable characters override this to read their data asset.
	return HitVoiceLines;
}

void ABaseCharacter::PlayHitReact()
{
	if (bIsDead)
	{
		return;
	}

	// flinch montage - naturally interrupts an in-progress attack montage (getting hit breaks your swing)
	if (UAnimMontage* Montage = GetHitReactMontage())
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			if (UAnimInstance* Anim = MeshComp->GetAnimInstance())
			{
				Anim->Montage_Play(Montage);
			}
		}
	}

	// hit voice - a random line, played even if there's no flinch montage
	const TArray<TObjectPtr<USoundBase>>& Voices = GetHitVoiceLines();
	if (Voices.Num() > 0)
	{
		const int32 Index = FMath::RandRange(0, Voices.Num() - 1);
		if (USoundBase* Voice = Voices[Index])
		{
			UGameplayStatics::SpawnSoundAttached(Voice, GetMesh());
		}
	}
}
