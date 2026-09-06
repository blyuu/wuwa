# 팀 편성 & 캐릭터 교체 (변주 게이지 · 충전 교체)

## 왜 필요했나
명조처럼 **3명을 편성해 실시간으로 교체**하며 싸우는 것이 목표였습니다. 이때 각 캐릭터의 상태(HP 등)는 독립적으로 유지되어야 하고, 그냥 바꾸는 것 말고 "타이밍 교체"에 보상을 주고 싶었습니다.

## 무엇을 설계했나
- **일반 교체** — 즉시 캐릭터 전환
- **충전 교체** — 변주 게이지가 가득 찼을 때 교체하면 추가 피해 + 등·퇴장 연출

## 어떻게 설계했나
`TeamComponent`가 3명을 **미리 스폰**(각자 ASC·AttributeSet 보유)해두고, 활성 캐릭터만 Possess로 조종합니다(오브젝트 풀링과 유사). 각 캐릭터가 독립 ASC라 HP·게이지·버프가 교체를 넘어 유지됩니다.

**일반 교체**는 즉시 Possess를 넘깁니다. **충전 교체**는 변주 게이지가 가득 찬 상태에서 교체할 때 분기하며, 타이머로 3박자를 오케스트레이션합니다.
1. 최근접 적에게 **대상 최대 체력의 N%**(`ChargedSwapDamagePercent`, 기본 5%) 고정 피해 + 게이지 소모.
2. 나가는 캐릭터가 약 1초(`ChargedSwapOutroTime`) 남아 퇴장 평타 — 이 아웃트로 동안 들어온 교체 입력은 `PendingSwapIndex` 가드로 잠급니다(연출 꼬임 방지).
3. 타이머 만료 후 실제 교체 → 들어오는 캐릭터가 적의 좌/우/뒤 랜덤 위치로 등장(`GA_Intro`).

고정 피해는 **새 데미지 경로를 만들지 않고** 기존 SetByCaller 데미지 GE(`Data.Damage`)를 그대로 재사용합니다 — 값만 "대상 MaxHp × N%"로 주입할 뿐이라, 01의 단일 초크 포인트를 그대로 통과해 그로기 배수·사망 처리 같은 공통 규칙이 저절로 적용됩니다. 별도 충전 교체용 데미지 어빌리티를 만들었다면 이 규칙들을 또 복제해야 했을 겁니다.

각 캐릭터가 독립 액터라 HP는 교체를 넘어 유지되어야 하는데, `PossessedBy`가 교체마다 호출되며 HP를 풀피로 재초기화하던 버그가 있었습니다. **어트리뷰트 초기화를 캐릭터당 최초 1회로 가드**해 해결했습니다(HP뿐 아니라 변주 게이지 초기화도 이 가드 안에 함께 둡니다).

```cpp
void APlayableCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (!bAttributesInitialized && CharacterData)   // 교체(재빙의) 시 리셋 방지 — 최초 1회만
    {
        bAttributesInitialized = true;
        InitializeAttributes(CharacterData->MaxHp);
        InitVariationEnergy(0.f, CharacterData->MaxVariationEnergy);   // 게이지도 이 가드 안에서
    }
}
```

**왜 미리 스폰 + Possess였나** — 교체마다 캐릭터를 스폰/파괴하면 HP·게이지·버프 같은 상태가 매번 날아갑니다. 셋을 미리 만들어 두고 조종권(Possess)만 넘기면 벤치 캐릭터의 상태가 그대로 살아있고, 서포터 팀 버프(06)도 벤치 포함 전원에 걸 수 있습니다.
