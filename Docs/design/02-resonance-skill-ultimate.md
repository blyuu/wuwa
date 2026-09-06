# 레조넌스 스킬 & 궁극기 (Resonance Skill / Liberation)

## 왜 필요했나
캐릭터마다 스킬과 궁극기의 모션·데미지·연출이 전부 다릅니다. 그런데 스킬과 궁극기는 **흐름이 거의 같습니다** — 소프트 락온으로 적을 보고, 몽타주를 재생하고, 타격 프레임에 데미지를 적용하고, 보이스를 재생합니다. 이걸 각각 따로 만들면 같은 코드가 반복됩니다.

## 무엇을 설계했나
스킬과 궁극기를 **하나의 어빌리티 베이스로 통합**하고, 캐릭터별로 달라지는 부분은 데이터에서 조회만 하도록 했습니다.

## 어떻게 설계했나
`GA_ResonanceSkill`이 자신의 Asset Tag로 `CharacterData->Skills[Tag]`를 조회해 몽타주를 재생합니다. 데미지는 새 경로를 만들지 않고 평타와 **동일한 히트 이벤트(`Event.Attack.Hit`)** 를 기다렸다가 적용하므로, 판정/적용 분리 구조를 그대로 재사용합니다. 여기서 덕을 봅니다 — 스킬·궁극기 몽타주에 `WeaponAnimNotifyState`만 얹으면(`HitEventTag`를 비워 두면 자동으로 `Event.Attack.Hit`로 폴백), **코드 한 줄 없이** 기존 데미지 파이프라인에 그대로 연결됩니다.

궁극기(`GA_Liberation`)는 이 클래스를 **상속**받아 딱 두 가지만 추가합니다.
- **발동 조건** — 게이지가 가득 찼을 때(`Energy >= Max - 0.01f`)만.
- **무적 시전** — 생성자에서 `ActivationOwnedTags`에 `State.Invulnerable`을 넣어, **궁극기가 재생되는 동안 내내 무적**입니다(시네마틱 연출 중 피격 방지). 어빌리티가 끝나면 GAS가 태그를 자동 제거하고, 이 무적은 데미지 파이프라인의 `PreGameplayEffectExecute`가 그대로 존중합니다(01 문서).

그 외 흐름(몽타주·히트·데미지·보이스)은 부모와 완전히 동일합니다.

```cpp
// 궁극기 = 스킬과 동일한 흐름 + "게이지 풀" 조건 + 무적만 추가
UGA_Liberation::UGA_Liberation()
{
    ActivationOwnedTags.AddTag(StateTags::State_Invulnerable);   // 시전 내내 무적
}

bool UGA_Liberation::CanActivateAbility(...) const
{
    if (!Super::CanActivateAbility(...)) return false;          // 쿨타임 등 공통 조건
    const float E   = ASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetUltimateEnergyAttribute());
    const float Max = ASC->GetNumericAttribute(UWuWa_AttributeSetBase::GetMaxUltimateEnergyAttribute());
    return Max > 0.f && E >= Max - 0.01f;                       // 가득 찼을 때만 (float 오차 여유)
}

void UGA_Liberation::ActivateAbility(...)
{
    ASC->ApplyModToAttribute(UWuWa_AttributeSetBase::GetUltimateEnergyAttribute(),
                             EGameplayModOp::Override, 0.f);      // 소모
    Super::ActivateAbility(...);                                  // 나머지는 스킬과 동일
}
```

**왜 상속이었나** — 스킬과 궁극기의 "다른 점"은 발동 조건과 무적 두 가지뿐이고 나머지는 같습니다. 공통 흐름을 부모에 두고 차이만 자식에서 얹으니, 몽타주·히트·데미지 로직을 한 벌만 유지하면 됩니다.
