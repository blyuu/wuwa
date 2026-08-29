// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerAnimInstance.h"

#include "Character/BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	APawn* OwnerPawn = TryGetPawnOwner();
	if (IsValid(OwnerPawn) == true)
	{
		OwnerCharacter = Cast<ABaseCharacter>(OwnerPawn);
		OwnerCharacterMovement = OwnerCharacter->GetCharacterMovement();
	}
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 런타임 스폰된 적은 init 시점에 소유 폰이 아직 없어 캐시에 실패할 수 있다.
	// 그런 경우 여기서 매 프레임 다시 잡아준다 (한 번 잡히면 유지됨).
	if (IsValid(OwnerCharacter) == false)
	{
		OwnerCharacter = Cast<ABaseCharacter>(TryGetPawnOwner());
		if (IsValid(OwnerCharacter) == true)
		{
			OwnerCharacterMovement = OwnerCharacter->GetCharacterMovement();
		}
	}

	if (IsValid(OwnerCharacter) == false ||
	 IsValid(OwnerCharacterMovement) == false)
	{
		return;
	}
	
	if (IsValid(OwnerCharacter) == true && 
		IsValid(OwnerCharacterMovement) == true)
	{
		Velocity = OwnerCharacterMovement->Velocity;
		GroundSpeed = UKismetMathLibrary::VSizeXY(Velocity);
		
		float GroundAcceleration = UKismetMathLibrary::VSizeXY(OwnerCharacterMovement->GetCurrentAcceleration());
		bool bIsAccelerationNearlyZero = FMath::IsNearlyZero(GroundAcceleration);
		bShouldMove = (KINDA_SMALL_NUMBER < GroundSpeed) && (bIsAccelerationNearlyZero == false);
	}
	
	bIsFalling = OwnerCharacterMovement->IsFalling();
	
}
