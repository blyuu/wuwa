# 쿨타임 시스템

## 왜 필요했나
스킬과 궁극기는 연타를 막아야 하고, 남은 시간을 UI로 보여줘야 합니다. 어빌리티마다 타이머를 직접 굴리면 코드가 중복되고 UI 동기화가 어렵습니다.

## 무엇을 설계했나
**데이터로 쿨타임 길이를 정하고**, 재시전 차단과 UI 표시를 GAS 위에서 일관되게 처리하도록 했습니다.

## 어떻게 설계했나
지속형 GameplayEffect(`GE_CoolDown`)의 길이를 **SetByCaller로 주입**해 스킬 데이터(`FSkillData::Cooldown`) 값을 그대로 쓰고, 시전 시 `Cooldown.ResonanceSkill` / `Cooldown.Liberation` 태그를 동적으로 부여합니다. 쿨타임 값이 0이면(예: 쿨 없는 평타) GE와 태그를 아예 적용하지 않습니다. `CanActivateAbility`가 이 태그를 검사해 재시전을 막고, HUD는 남은 시간을 읽어 라디얼과 카운트다운을 그립니다.

이 한 클래스(`GE_CoolDown`)를 스킬·궁극기뿐 아니라 **서포터 버프(`GA_TeamBuff`)까지 같은 `Data.CooldownDuration` 경로로 재사용**합니다 — 쿨타임이 필요한 모든 어빌리티가 코드 복제 없이 동일한 방식으로 동작합니다.

```cpp
// GE_CoolDown: 길이는 시전할 때 주입 (SetByCaller)
UGE_CoolDown::UGE_CoolDown()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    FSetByCallerFloat SetByCaller; SetByCaller.DataTag = DataTags::Data_CooldownDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}

// 시전: 쿨타임 값이 있을 때만 GE + 태그 적용
if (CooldownEffect && CooldownTag.IsValid() && Skill->Cooldown > 0.f)
{
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(CooldownEffect, 1.f, Ctx);
    Spec.Data->SetSetByCallerMagnitude(DataTags::Data_CooldownDuration, Skill->Cooldown);
    Spec.Data->DynamicGrantedTags.AddTag(CooldownTag);      // 예: Cooldown.ResonanceSkill
    ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

// 재시전 차단
if (ASC->HasMatchingGameplayTag(CooldownTag)) return false;

// HUD: 여러 쿨다운 이펙트 중 "가장 오래 남은 것"을 골라 라디얼 + 숫자
float Remaining = 0.f, Duration = 0.f;
for (const TPair<float,float>& T : ASC->GetActiveEffectsTimeRemainingAndDuration(
        FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CooldownTag))))
{
    Remaining = FMath::Max(Remaining, T.Key);
    Duration  = FMath::Max(Duration,  T.Value);
}
const float Percent = (Duration > 0.f) ? Remaining / Duration : 0.f;   // 라디얼, 텍스트 = "%.1f"
```

**왜 GE + 태그였나** — 어빌리티마다 `FTimerHandle`을 직접 굴리면 "남은 시간"을 UI가 또 따로 추적해야 합니다. 쿨타임을 지속형 GE로 두면 남은 시간·비율을 GAS가 이미 알고 있어 HUD가 그대로 질의하면 되고, 재시전 차단도 "태그가 있나?" 한 줄로 끝납니다.
