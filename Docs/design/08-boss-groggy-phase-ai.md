# 보스전 — 그로기 & 페이즈 & AI

## 왜 필요했나
보스는 일반 몹과 달리 **그로기(무력화)** 와 **페이즈 전환**이 있어야 긴장감이 생깁니다. 두 시스템 모두 코드에 하드코딩하지 않고 데이터로 조절하고 싶었습니다.

## 무엇을 설계했나
평타를 누적해 보스를 무력화하는 그로기, HP 비율로 패턴이 바뀌는 페이즈, 그리고 이를 따르는 AI입니다.

## 어떻게 설계했나
`Groggy`를 HP와 같은 GAS 어트리뷰트로 두어 UI가 자동 갱신되고, "그로기 상태인가"는 값이 아니라 `Enemy.State.Groggy` 태그로 표현해 **데미지·AI·UI가 서로를 직접 참조하지 않고 각자 태그만 질의해 협력**합니다(데미지 계산은 태그가 있으면 `GroggyDamageMultiplier`(데이터, 기본 1.5)를 곱하고, 보스 체력바 UI는 태그로 취약 표시를 켭니다).

무력화는 두 단계로 나뉩니다.
- **스턴** — 평타로 그로기 게이지가 0이 되면 `StopLogic("Groggy")`로 AI를 멈추고, 스태거 몽타주를 재생하되 `GroggyLoopSection`을 자기 자신으로 루프시켜(주저앉기 1회 → 앉은 자세 반복) 회복까지 포즈를 유지합니다.
- **기상** — `bIsRecovering` 단계에서 회복 게이지가 시간에 걸쳐 다시 차고, 다 차면 기상(get-up) 몽타주를 재생한 뒤 그 **몽타주 종료 콜백에서 `RestartLogic()`** 으로 AI를 재개합니다(모션이 끝나기 전에 움직이지 않도록 재개 타이밍을 몽타주에 묶었습니다).

페이즈는 현재 HP 비율로 계산하고, `PerformAttack`이 **현재 페이즈에서 쓸 수 있는 스킬만**(`MinPhase <= 현재 페이즈`) 모아 랜덤으로 고른 뒤 그 스킬의 몽타주 목록에서 다시 랜덤 재생합니다. 이동·추격·공격은 Behavior Tree가 담당합니다.

```cpp
int32 AEnemyCharacter::GetCurrentPhase() const
{
    if (AttributeSet->GetMaxHp() <= 0.f) return 1;
    const float Ratio = AttributeSet->GetHp() / AttributeSet->GetMaxHp();
    int32 Phase = 1;
    for (const float T : EnemyDataAsset->PhaseHpRatios)   // 내림차순
        if (Ratio <= T) Phase++;
    return Phase;
}
```

**왜 값이 아니라 태그였나** — "그로기냐"를 각 시스템이 `bool`로 물어보게 하면 데미지·AI·UI가 서로를 직접 알아야 해 결합이 심해집니다. 상태를 태그로 표현하면 셋 다 **서로를 모른 채** 태그만 질의해 협력하고, 페이즈·회복 시간·취약 배수를 전부 데이터 에셋에서 조절할 수 있습니다.
