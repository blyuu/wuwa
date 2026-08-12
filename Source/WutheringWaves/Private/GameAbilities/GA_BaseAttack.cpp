#include "GameAbilities/GA_BaseAttack.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimationAsset.h"
#include "Character/BaseCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Character/WeaponClass.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_BaseAttack::UGA_BaseAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
}

void UGA_BaseAttack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	UE_LOG(LogTemp, Warning, TEXT("[Combo] InputPressed - window open: %s"), bWindowIsOpen ? TEXT("true") : TEXT("false"));

	if (bWindowIsOpen)
	{
		bWindowIsOpen = false;
		AdvanceCombo();
	}
	else
	{
		bComboInputBuffered = true;
	}
}

void UGA_BaseAttack::AdvanceCombo()
{
	CurrentComboIndex++;

	FName CurrentSection = FName(*FString::Printf(TEXT("Attack_%d"), CurrentComboIndex));
	FName NextSection    = FName(*FString::Printf(TEXT("Attack_%d"), CurrentComboIndex + 1));

	UE_LOG(LogTemp, Warning, TEXT("[Combo] %s -> %s"), *CurrentSection.ToString(), *NextSection.ToString());

	UAnimInstance* AnimInstance = GetCurrentActorInfo()->GetAnimInstance();

	if (CurrentMontage->GetSectionIndex(NextSection) != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(NextSection, CurrentMontage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combo] Section NOT found - no more combos"));
	}
}

void UGA_BaseAttack::OnComboWindowOpen(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Warning, TEXT("[Combo] Window opened - buffered: %s"), bComboInputBuffered ? TEXT("true") : TEXT("false"));

	if (bComboInputBuffered)
	{
		bComboInputBuffered = false;
		AdvanceCombo();
	}
	else
	{
		bWindowIsOpen = true;
	}
}

void UGA_BaseAttack::OnHitEventRecieved(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Display, TEXT("Hit! Combo %d"), CurrentComboIndex + 1);
	DoDamage(Payload);
}

void UGA_BaseAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ActorInfo->AvatarActor);
	
	BaseCharacter->CurrentWeapon->ShowWeapon();
	
	UE_LOG(LogTemp, Display, TEXT("Used BaseAttack"));
	
	FGameplayTag BaseAttackTag = GetAssetTags().First();
	
	if (!BaseCharacter->CharacterData)
	{
		return;
	}
	
	
	//변경 전 UAnimMontage* BaseAttackMontage = BaseCharacter->CharacterData->Skills[BaseAttackTag].Montage;
	
	//변경 후
	CurrentMontage = BaseCharacter->CharacterData->Skills[BaseAttackTag].Montage;
	UAnimMontage* BaseAttackMontage = CurrentMontage;
	
	UAbilityTask_PlayMontageAndWait* PlayBaseAttackMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,BaseAttackMontage);
	
	PlayBaseAttackMontageTask->OnBlendOut.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->OnCancelled.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->OnCompleted.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->OnInterrupted.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->ReadyForActivation();
	
	CurrentComboIndex = 0;
	bComboInputBuffered = false;
	bWindowIsOpen = false;

	// 콤보 윈도우 이벤트 대기
	UAbilityTask_WaitGameplayEvent* WaitCombo = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Combo_WindowOpen);
	WaitCombo->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnComboWindowOpen);
	WaitCombo->ReadyForActivation();

	// 피격 판정 이벤트 대기
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_BaseAttack_Hit);
	WaitHit->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnHitEventRecieved);
	WaitHit->ReadyForActivation();
	
}

void UGA_BaseAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(ActorInfo->AvatarActor);
	
	BaseCharacter->CurrentWeapon->HideWeapon();
	
	CurrentComboIndex = 0;
	bComboInputBuffered = false;
	bWindowIsOpen = false;
}

void UGA_BaseAttack::EndMontage()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}


FGameplayTag UGA_BaseAttack::GetComboTargetEventTag()
{
	return FGameplayTag::RequestGameplayTag("Event.BaseAttack.Hit");
}

void UGA_BaseAttack::DoDamage(FGameplayEventData Data)
{
	TArray<FHitResult> HitResults = GetHitResultFromSweepLocationTargetData(Data.TargetData, 15.f, true, true);

	for (const FHitResult& Hit : HitResults)
	{
		if (Hit.GetActor())
		{
			UE_LOG(LogTemp, Display, TEXT("[Hit] Actor: %s | Bone: %s | Location: %s"),
				*Hit.GetActor()->GetName(),
				*Hit.BoneName.ToString(),
				*Hit.Location.ToString());
		}
	}
}

TArray<FHitResult> UGA_BaseAttack::GetHitResultFromSweepLocationTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, float BladeHalfThickness, bool bDrawDebug, bool bIgnoreSelf) const
{
	TArray<FHitResult> OutResult;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor || TargetDataHandle.Data.Num() == 0) return OutResult;

	// 첫 번째(루트)와 마지막(팁) LocationInfo로 전체 블레이드 구성
	FVector BladeRoot = TargetDataHandle.Data[0]->GetOrigin().GetTranslation();
	FVector BladeTip  = TargetDataHandle.Data.Last()->GetEndPoint();

	float BladeLength = FVector::Dist(BladeRoot, BladeTip);
	if (BladeLength < KINDA_SMALL_NUMBER) return OutResult;

	FVector BladeDir    = (BladeTip - BladeRoot) / BladeLength;
	FVector BladeCenter = (BladeRoot + BladeTip) * 0.5f;

	// 박스 로컬 Z축을 블레이드 방향으로 정렬
	FQuat BladeOrient = FQuat::FindBetweenVectors(FVector::UpVector, BladeDir);
	FVector HalfSize(BladeHalfThickness, BladeHalfThickness, BladeLength * 0.5f);

	// 휘두르는 방향(캐릭터 오른쪽)으로 sweep → 궤적이 생김
	FVector SwingDir   = AvatarActor->GetActorRightVector();
	FVector SweepStart = BladeCenter - SwingDir * BladeHalfThickness;
	FVector SweepEnd   = BladeCenter + SwingDir * BladeHalfThickness;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	if (bIgnoreSelf) ActorsToIgnore.Add(AvatarActor);

	EDrawDebugTrace::Type DrawType = bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	TArray<FHitResult> Results;
	UKismetSystemLibrary::BoxTraceMultiForObjects(
		AvatarActor, SweepStart, SweepEnd,
		HalfSize, BladeOrient.Rotator(),
		ObjectTypes, false, ActorsToIgnore,
		DrawType, Results, false,
		FLinearColor::Red, FLinearColor::Green, 5.f
	);

	OutResult.Append(Results);
	return OutResult;
}
