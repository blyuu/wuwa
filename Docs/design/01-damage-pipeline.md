# 데미지 일원화 — AttributeSet 단일 초크 포인트

## 왜 필요했나
이 게임은 평타·스킬·궁극기·적 공격·충전 교체 등 **데미지가 발생하는 지점이 매우 많습니다.** 그런데 "맞으면 HP가 깎인다" 외에도 모든 공격이 공통으로 따라야 하는 규칙이 있습니다 — 무적(i-frame)이면 피해 무시, 그로기 상태면 피해 증가, 공격자의 공격력 배수 반영, HP가 0이면 사망 처리, 데미지 숫자 표시, 타격음 재생 등입니다. 이 규칙들을 각 공격 로직에 흩뿌리면 공격이 늘어날 때마다 같은 코드를 복사하게 되고, 한 곳만 고쳐도 다른 곳에서 누락되어 버그가 납니다.

## 무엇을 설계했나
**모든 데미지가 반드시 통과하는 단 하나의 지점**을 만들고, 공통 전투 규칙을 전부 그 지점에서만 처리하도록 했습니다.

## 어떻게 설계했나
핵심은 `Damage`를 **실제로 저장되는 스탯이 아니라 "이번 타격량"만 실어 나르는 메타 어트리뷰트**로 둔 것입니다. 데미지 GameplayEffect가 이 값을 건드리면 `AttributeSet`가 한 번씩 "정산"하고 값을 0으로 비웁니다. 처리는 두 단계로 나눴습니다.

- `PreGameplayEffectExecute()` — 적용 **전**에 `State.Invulnerable`(무적/궁극기 시전 등) 태그가 있으면 이펙트 자체를 거부해 HP를 아예 건드리지 않습니다. i-frame이 여기서 처리됩니다.
- `PostGameplayEffectExecute()` — 실제 종착점. 공격력 배수(서포터 버프가 올린 `AttackPower`) → 그로기 취약(적 데이터의 `GroggyDamageMultiplier`, 기본 1.5) → HP 반영 → 사망/피격 리액션 → 데미지 넘버·타격음까지 한 번에 처리합니다.

```cpp
// 적용 전: 무적이면 데미지 이펙트 자체를 거부 (i-frame)
bool UWuWa_AttributeSetBase::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
    if (Data.Target.HasMatchingGameplayTag(StateTags::State_Invulnerable))
        return false;                        // HP를 건드리기 전에 통째로 차단
    return Super::PreGameplayEffectExecute(Data);
}

// 종착점: 모든 데미지가 여기서 "정산"된다
void UWuWa_AttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    if (Data.EvaluatedData.Attribute != GetDamageAttribute()) return;

    float Dmg = GetDamage();
    SetDamage(0.f);                          // 메타 어트리뷰트 → 읽고 바로 비움

    // 공격자의 공격력 배수 (1.0 = 버프 없음) — 서포터 버프가 이 값을 올린다
    if (UAbilitySystemComponent* Src = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent())
    {
        const float Atk = Src->GetNumericAttribute(GetAttackPowerAttribute());
        if (Atk > 0.f) Dmg *= Atk;
    }

    // 그로기(취약) 상태면 데이터로 정한 배수 (기본 1.5)
    float Mult = 1.5f;
    if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Character))
        if (Enemy->EnemyDataAsset) Mult = Enemy->EnemyDataAsset->GroggyDamageMultiplier;
    if (Data.Target.HasMatchingGameplayTag(StateTags::Enemy_State_Groggy))
        Dmg *= Mult;

    const float NewHp = FMath::Max(GetHp() - Dmg, 0.f);
    SetHp(NewHp);

    if (NewHp <= 0.f)          Character->HandleDeath();
    else                       Character->PlayHitReact();   // 피격 리액션
    // + 데미지 넘버 리포트 / 타격음 (07 · 11 문서 참고)
}
```

**왜 메타 어트리뷰트였나** — HP 같은 저장 스탯에서 데미지를 직접 빼면 "이번 한 방이 얼마였는지"를 배수·피드백 계산이 다시 알아낼 방법이 없습니다. `Damage`를 매번 0으로 비우는 임시 채널로 두면, GE가 값을 실어 보낼 때마다 `PostGameplayEffectExecute`가 정확히 한 번 가로채 정산할 수 있고, 가짜 스탯을 리플리케이트하거나 클램프할 필요도 없습니다.

**결과:** 새 공격을 추가할 때는 "데미지 GE를 쏜다"까지만 하면 무적 판정·공격력 배수·그로기 배수·사망·피드백이 자동으로 따라옵니다. 공통 규칙을 바꿀 때도 이 한두 곳만 수정합니다.
