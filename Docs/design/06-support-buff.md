# 버프 서포터 캐릭터 (팀 공격력 버프)

## 왜 필요했나
딜러만이 아니라 **팀 전체를 강화하는 서포터**를 넣고 싶었습니다. 문제는 "공격력 증가"를 어디에 저장하느냐였습니다 — 지속 시간 동안 유지되고 데미지 계산이 읽을 수 있어야 하는데, GAS에서 그 저장소는 결국 어트리뷰트입니다.

## 무엇을 설계했나
E를 누르면 **팀 전원의 공격력이 일정 시간 상승**하는 버프입니다.

## 어떻게 설계했나
팀은 독립 ASC 3개이므로, `GetTeam()`을 순회하며 벤치 캐릭터를 포함한 전원에게 지속형 버프 GE를 겁니다(교체해도 유지). 데미지 배수를 위해 `AttackPower` 어트리뷰트(기본 1.0)를 두고, 데미지 일원화 지점(01)에서 공격자의 AttackPower를 곱합니다. 버프량과 지속 시간은 상수가 아니라 **스킬 데이터(`FSkillData::BuffAmount / BuffDuration`)에서 SetByCaller로 주입**합니다.

`GE_AttackBuff`는 `AttackPower`에 값을 **더하는(Additive)** 지속 효과라, 만료되면 GAS가 자동으로 1.0으로 되돌립니다 — 수동으로 버프를 "해제"하는 코드가 필요 없습니다. 또 `StackLimitCount = 1` + `RefreshOnSuccessfulApplication`으로 두어, E를 연타해도 **스택이 쌓이지 않고 지속 시간만 갱신**됩니다.

```cpp
void UGA_TeamBuff::ApplyTeamBuff()
{
    for (APlayableCharacter* Mate : Team->GetTeam())      // 벤치 포함 전원 (시전자 자신도 포함)
    {
        FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(BuffEffect, 1.f, Ctx);
        Spec.Data->SetSetByCallerMagnitude(DataTags::Data_AttackBuff,   Skill->BuffAmount);   // 데이터 값
        Spec.Data->SetSetByCallerMagnitude(DataTags::Data_BuffDuration, Skill->BuffDuration);
        Mate->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}
```

**왜 어트리뷰트 + 지속 GE였나** — "공격력 +30%"를 캐릭터에 `bool`/`float`로 들고 있으면, 시간이 끝났을 때 되돌리는 것을 **직접 기억해서 해제**해야 하고 그걸 놓치면 버프가 영구히 남습니다. Additive 지속 GE로 두면 만료 시 GAS가 알아서 원복하고, 스택 정책까지 데이터로 정할 수 있습니다.
