#include "Framework/LiberationCameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

ALiberationCameraActor::ALiberationCameraActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	GetCameraComponent()->SetFieldOfView(43.f);
}

void ALiberationCameraActor::Initialize(ACharacter* Character)
{
	Subject = Character;
	if (!Character) return;
	AddTickPrerequisiteActor(Character);
	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		AddTickPrerequisiteComponent(Mesh);
	}
	UpdateFraming();
}

void ALiberationCameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFraming();
}

void ALiberationCameraActor::UpdateFraming()
{
	ACharacter* Character = Subject.Get();
	if (!Character) return;
	const float Scale = FMath::Max(.1f, Character->GetCapsuleComponent()->GetComponentScale().GetAbsMax());
	FVector Face = Character->GetActorLocation() + FVector::UpVector * (150.f * Scale);
	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		static const FName HeadBone(TEXT("Bip001Head"));
		if (Mesh->GetBoneIndex(HeadBone) != INDEX_NONE)
		{
			Face = Mesh->GetBoneLocation(HeadBone) + FVector::UpVector * (2.f * Scale);
		}
	}
	// Restore the original portrait distance, following animated translation without
	// inheriting the head's roll/pitch (which would spin the camera during the attack).
	const FVector Position = Face + Character->GetActorForwardVector() * (110.f * Scale)
		+ Character->GetActorRightVector() * (38.f * Scale) + FVector::UpVector * (8.f * Scale);
	SetActorLocationAndRotation(Position, (Face - Position).Rotation());
}
