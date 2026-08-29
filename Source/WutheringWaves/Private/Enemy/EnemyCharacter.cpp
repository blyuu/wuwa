// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DataAsset/EnemyDataAsset.h"
#include "Enemy/WuwaEnemyController.h"
#include "GameAbilities/GA_EnemyAttack.h"
#include "GameAbilities/WuWa_AttributeSetBase.h"

AEnemyCharacter::AEnemyCharacter()
{
	// possess 배관을 C++에 박아 BP 세팅 실수를 원천 차단한다.
	// (지난번 "부여가 안 됨"이 possess 미발생 때문이었던 걸 방지)
	AIControllerClass = AWuwaEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	
	if (EnemyDataAsset)
	{
		InitializeAttributes(EnemyDataAsset->MaxHp);

		// 이동 속도도 데이터 에셋에서 받는다 (몬스터마다 다른 속도 가능).
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->MaxWalkSpeed = EnemyDataAsset->MoveSpeed;
		}
	}

	// 적은 AIController possess에 의존하지 않고 여기서 직접 어빌리티를 부여한다.
	// (bAbilitiesGranted 가드가 있어 이후 PossessedBy가 겹쳐도 중복 부여되지 않는다)
	GiveAbilites();
}

int32 AEnemyCharacter::GetCurrentPhase() const
{
	if (!AttributeSet || !EnemyDataAsset)
	{
		return 1;
	}

	const float MaxHpValue = AttributeSet->GetMaxHp();
	if (MaxHpValue <= 0.f)
	{
		return 1;
	}

	const float Ratio = AttributeSet->GetHp() / MaxHpValue;

	// PhaseHpRatios는 내림차순. 통과한 임계값 개수만큼 페이즈가 올라간다.
	int32 Phase = 1;
	for (const float Threshold : EnemyDataAsset->PhaseHpRatios)
	{
		if (Ratio <= Threshold)
		{
			Phase++;
		}
	}
	return Phase;
}

void AEnemyCharacter::PerformAttack()
{
	if (!AbilitySystemComponent || !EnemyDataAsset)
	{
		return;
	}

	const int32 Phase = GetCurrentPhase();

	// 1단계: 현재 페이즈에서 쓸 수 있는(MinPhase <= Phase) 공격 타입 태그들을 모은다.
	// 몽타주가 하나도 없는 타입은 건너뛴다.
	TArray<FGameplayTag> EligibleTags;
	for (const TPair<FGameplayTag, FEnemySkillData>& Skill : EnemyDataAsset->Skills)
	{
		if (Skill.Value.MinPhase <= Phase && Skill.Value.Montages.Num() > 0)
		{
			EligibleTags.Add(Skill.Key);
		}
	}

	if (EligibleTags.Num() == 0)
	{
		return;
	}

	// 태그 하나를 랜덤 선택 (이후 거리/가중치 기반으로 확장 가능).
	CurrentSkillTag = EligibleTags[FMath::RandRange(0, EligibleTags.Num() - 1)];

	// 2단계: 그 태그의 몽타주 목록에서 다시 랜덤으로 하나 뽑는다.
	const FEnemySkillData& Skill = EnemyDataAsset->Skills[CurrentSkillTag];
	CurrentMontage = Skill.Montages[FMath::RandRange(0, Skill.Montages.Num() - 1)];

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] Phase %d -> skill %s"), Phase, *CurrentSkillTag.ToString());

	// GA_EnemyAttack(또는 그 BP 자식)을 클래스 계층(IsA)으로 찾아 발동한다.
	// TryActivateAbilityByClass는 정확히 같은 클래스만 매칭해서 BP 자식이면 실패한다.
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->IsA(UGA_EnemyAttack::StaticClass()))
		{
			AbilitySystemComponent->TryActivateAbility(Spec.Handle);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Enemy] GA_EnemyAttack not granted"));
}

void AEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();
	Destroy();
}
