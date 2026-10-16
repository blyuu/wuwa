#include "GameAbilities/GA_BaseAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimationAsset.h"
#include "Character/BaseCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "DataAsset/CharacterDataAsset.h"
#include "Character/PlayableCharacter.h"
#include "Character/WeaponClass.h"
#include "DataAsset/EnemyDataAsset.h"
#include "Enemy/EnemyCharacter.h"
#include "Framework/CombatFeedbackSubsystem.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_BaseAttack::UGA_BaseAttack()
{
	// 액터마다 인스턴스를 하나씩 둬서, 콤보 진행 상태(CurrentComboIndex 등) 같은 멤버 변수를 캐릭터별로 안전하게 유지한다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

}

void UGA_BaseAttack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	UE_LOG(LogTemp, Warning, TEXT("[Combo] InputPressed - window open: %s"), bWindowIsOpen ? TEXT("true") : TEXT("false"));

	if (bWindowIsOpen)
	{
		// 콤보 입력 윈도우가 열려 있으면 바로 다음 콤보로 이어진다.
		bWindowIsOpen = false;
		AdvanceCombo();
	}
	else
	{
		// 윈도우가 아직 안 열렸다면 입력을 버퍼에 저장해뒀다가, 윈도우가 열리는 순간(OnComboWindowOpen) 소비한다.
		bComboInputBuffered = true;
	}
}

void UGA_BaseAttack::AdvanceCombo()
{
	if (!CurrentMontage || !GetCurrentActorInfo()) return;
	UAnimInstance* AnimInstance = GetCurrentActorInfo()->GetAnimInstance();
	if (!AnimInstance) return;
	const FName CurrentSection = AnimInstance->Montage_GetCurrentSection(CurrentMontage);
	const FString SectionName = CurrentSection.ToString();
	if (!SectionName.StartsWith(TEXT("Attack_"))) return;
	const int32 Index = FCString::Atoi(*SectionName.Mid(7));
	const FName NextSection(*FString::Printf(TEXT("Attack_%d"), Index + 1));
	if (CurrentMontage->GetSectionIndex(NextSection) == INDEX_NONE) return;

	// A section with no next link begins blending out before its end. Jump while the
	// combo window is open, as the working combo did before the movement change.
	AnimInstance->Montage_JumpToSection(NextSection, CurrentMontage);
	CurrentComboIndex = Index + 1;
	bWindowIsOpen = false;
	bComboInputBuffered = false;
	if (APlayableCharacter* Character = Cast<APlayableCharacter>(GetAvatarActorFromActorInfo()))
		Character->FaceTargetForAttack();
	UE_LOG(LogTemp, Warning, TEXT("[Combo] %s -> %s"), *CurrentSection.ToString(), *NextSection.ToString());
}

void UGA_BaseAttack::OnComboWindowOpen(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Warning, TEXT("[Combo] Window opened - buffered: %s"), bComboInputBuffered ? TEXT("true") : TEXT("false"));

	if (bComboInputBuffered)
	{
		// 윈도우가 열리기 전에 미리 눌러둔 입력이 있으면 그걸 바로 소비해서 다음 콤보로 넘어간다.
		bComboInputBuffered = false;
		AdvanceCombo();
	}
	else
	{
		// 아직 입력이 없으면 윈도우를 열어두고 다음 입력(InputPressed)을 기다린다.
		bWindowIsOpen = true;
	}
}


void UGA_BaseAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(ActorInfo->AvatarActor);

	// 무기 장착/수납은 이제 몽타주에 붙은 AN_WeaponVisibility 노티파이가 처리한다 (여기서 직접 하지 않음)

	UE_LOG(LogTemp, Display, TEXT("Used BaseAttack"));

	FGameplayTag BaseAttackTag = GetAssetTags().First();

	if (!BaseCharacter || !BaseCharacter->CharacterData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 캐릭터 데이터 에셋에서 이 평타 태그에 해당하는 몽타주를 찾아온다 (캐릭터마다 다른 평타 애니메이션을 쓰기 위함)
	const FSkillData* Skill = BaseCharacter->CharacterData->Skills.Find(BaseAttackTag);
	CurrentMontage = Skill ? Skill->Montage : nullptr;
	if (!CurrentMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// Acquire a nearby enemy before the first swing, then keep tracking it through
	// subsequent combo sections until the ability ends or the target is lost.
	BaseCharacter->FaceTargetForAttack();
	UAnimMontage* BaseAttackMontage = CurrentMontage;

	UAbilityTask_PlayMontageAndWait* PlayBaseAttackMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this,NAME_None,BaseAttackMontage);

	// 몽타주가 어떤 식으로 끝나든(정상 종료/캔슬/인터럽트/블렌드아웃) 전부 같은 정리 루틴(EndMontage)으로 받는다.
	PlayBaseAttackMontageTask->OnBlendOut.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->OnCancelled.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->OnCompleted.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->OnInterrupted.AddDynamic(this,&UGA_BaseAttack::EndMontage);
	PlayBaseAttackMontageTask->ReadyForActivation();
	if (!IsActive()) return;

	CurrentComboIndex = 0;
	bComboInputBuffered = false;
	bWindowIsOpen = false;

	// 콤보 입력 윈도우 이벤트 - 몽타주의 AnimNotifyState가 구간 동안 열어줌
	UAbilityTask_WaitGameplayEvent* WaitCombo = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Combo_WindowOpen);
	WaitCombo->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnComboWindowOpen);
	WaitCombo->ReadyForActivation();

	// 피격 판정 이벤트 - WeaponAnimNotifyState가 무기 소켓 스윕으로 적중을 감지하면 쏨
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Attack_Hit);
	WaitHit->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnHitEvent);
	WaitHit->ReadyForActivation();

	// 보이스 라인 이벤트 - 각 콤보 섹션에 붙은 AnimNotify가 쏴서, 타격마다 보이스가 나올 수 있게 한다
	UAbilityTask_WaitGameplayEvent* WaitVoice = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Skill_Voice);
	WaitVoice->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnVoiceEvent);
	WaitVoice->ReadyForActivation();

	// VFX 이벤트 - 몽타주의 AN_SendGameplayEvent 노티파이가 쏘면 -> 해당 인덱스의 Vfx[] 항목을 스폰한다
	UAbilityTask_WaitGameplayEvent* WaitVfx = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, EventTags::Event_Skill_Vfx);
	WaitVfx->EventReceived.AddDynamic(this, &UGA_BaseAttack::OnVfxEvent);
	WaitVfx->ReadyForActivation();
}

void UGA_BaseAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 타겟 어시스트/무기 상태 등 평타 중에만 유효했던 상태를 되돌린다.
	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(ActorInfo->AvatarActor);
	if (BaseCharacter)
	{
		BaseCharacter->CancelTargetAssist();
		if (BaseCharacter->CurrentWeapon) BaseCharacter->CurrentWeapon->HideWeapon();
	}

	CurrentComboIndex = 0;
	bComboInputBuffered = false;
	bWindowIsOpen = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_BaseAttack::EndMontage()
{
	// 몽타주 델리게이트(OnBlendOut/OnCancelled/OnCompleted/OnInterrupted) 네 가지가 공통으로 들어오는 지점.
	// 이미 끝난 어빌리티에서 중복 호출될 수 있으므로 IsActive() 체크로 막는다.
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}


void UGA_BaseAttack::OnHitEvent(FGameplayEventData Payload)
{
	// WeaponAnimNotifyState(근접) 또는 원거리 트레이스가 적중을 보고하면 호출된다.
	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!BaseCharacter || !BaseCharacter->CharacterData) return;

	if (BaseCharacter->CharacterData->RangeTag == RangeTags::Character_Range_Ranged)
	{
		// 원거리 캐릭터는 노티파이가 보낸 Target을 쓰지 않고, 이 시점에 직접 카메라 기준 라인트레이스를 쏜다.
		PerformRangedTrace();
	}
	else
	{

		AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());
		if (HitActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Melee] Hit confirmed: %s"), *HitActor->GetName());
			// 패링은 평타 입력을 받아들이는 시점에서 이미 판정이 끝나 있고, 여기서는 데미지 처리만 한다.
			AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(HitActor);

			// 적이 데이터 에셋에 설정된 확률로 이 타격을 뒤로 빠지며 회피할 수 있다. 회피에 성공하면
			// 데미지/그로기/게이지 획득 전부 무효가 되므로 여기서 바로 리턴한다.
			if (Enemy)
			{
				if (Enemy->TryDodge()) return;
			}

			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

			FGameplayTag BaseAttackTag = GetAssetTags().First();
			const FSkillData& SkillData = BaseCharacter->CharacterData->Skills[BaseAttackTag];
			TSubclassOf<UGameplayEffect> DamageEffectClass = SkillData.DamageEffect;

			if (TargetASC && DamageEffectClass)
			{
				FGameplayEffectContextHandle Context = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();

				FGameplayEffectSpecHandle Spec = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
					// 데이터 기반 데미지: 수치는 GE 안이 아니라 스킬 데이터(Skills[tag].Damage)에 있고, SetByCaller로 그때그때 주입한다.
					Spec.Data->SetSetByCallerMagnitude(DataTags::Data_Damage, SkillData.Damage);

				TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}

			// 이 스킬의 그로기 수치만큼 적의 그로기 게이지를 깎는다
			if (Enemy)
			{
				Enemy->ApplyGroggyDamage(SkillData.GroggyDamage);
			}

			// 적중이 확정됐으니 변주 게이지(충전 교체용)를 채운다
			if (SkillData.VariationGain > 0.f)
			{
				GetAbilitySystemComponentFromActorInfo()->ApplyModToAttribute(
					UWuWa_AttributeSetBase::GetVariationEnergyAttribute(),
					EGameplayModOp::Additive, SkillData.VariationGain);
			}

			// 적중이 확정됐으니 궁극기 효율 게이지도 채운다
			if (SkillData.UltimateGain > 0.f)
			{
				GetAbilitySystemComponentFromActorInfo()->ApplyModToAttribute(
					UWuWa_AttributeSetBase::GetUltimateEnergyAttribute(),
					EGameplayModOp::Additive, SkillData.UltimateGain);
			}
		}
	}
}

void UGA_BaseAttack::OnVoiceEvent(FGameplayEventData Payload)
{
	PlaySkillVoice();
}

void UGA_BaseAttack::PlaySkillVoice()
{
	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!BaseCharacter || !BaseCharacter->CharacterData) return;

	const FSkillData& Skill = BaseCharacter->CharacterData->Skills[GetAssetTags().First()];
	if (Skill.VoiceLines.Num() == 0) return;

	// 매번 같은 콤보라도 똑같은 보이스만 나오지 않도록 무작위로 하나 고른다
	const int32 Index = FMath::RandRange(0, Skill.VoiceLines.Num() - 1);
	if (USoundBase* Voice = Skill.VoiceLines[Index])
	{
		UGameplayStatics::SpawnSoundAttached(Voice, BaseCharacter->GetMesh());
	}
}

void UGA_BaseAttack::OnVfxEvent(FGameplayEventData Payload)
{
	// 노티파이가 EventMagnitude에 실어보낸 숫자가 곧 Skill.Vfx 배열의 인덱스다
	PlaySkillVfx(FMath::RoundToInt(Payload.EventMagnitude));
}

void UGA_BaseAttack::PlaySkillVfx(int32 VfxIndex)
{
	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor);
	if (!BaseCharacter || !BaseCharacter->CharacterData) return;

	const FSkillData& Skill = BaseCharacter->CharacterData->Skills[GetAssetTags().First()];
	if (!Skill.Vfx.IsValidIndex(VfxIndex)) return;

	const FSkillVfxData& Vfx = Skill.Vfx[VfxIndex];
	BaseCharacter->PlayAttachedNiagaraEffect(Vfx.Effect, Vfx.bAttachToWeapon, Vfx.SocketName,
		Vfx.LocationOffset, Vfx.RotationOffset);
}

void UGA_BaseAttack::PerformRangedTrace()
{
	// 원거리 평타 전용 적중 판정: 무기 노티파이 대신, 현재 카메라 시점에서 직접 라인트레이스를 쏜다.
	APlayableCharacter* BaseCharacter = Cast<APlayableCharacter>(GetCurrentActorInfo()->AvatarActor);

	const APlayerController* PC = Cast<APlayerController>(BaseCharacter->GetController());
	if (!PC)
	{
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	// 카메라 위치에서 바라보는 방향으로 5000cm만큼 트레이스
	const FVector TraceEnd = CameraLocation + CameraRotation.Vector() * 5000.f;

	FHitResult HitResult;
	TArray<AActor*> ActorsToIgnore = { BaseCharacter };

	UKismetSystemLibrary::LineTraceSingle(
		BaseCharacter,
		CameraLocation,
		TraceEnd,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResult,
		true
	);

	if (HitResult.bBlockingHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Ranged] Hit: %s"), *HitResult.GetActor()->GetName());
	}
}

FGameplayTag UGA_BaseAttack::GetComboTargetEventTag()
{
	// 외부(예: 타겟 어시스트 로직)에서 "이 어빌리티의 적중 이벤트 태그가 뭔지" 조회할 때 쓰는 헬퍼
	return EventTags::Event_Attack_Hit;
}

