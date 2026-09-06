# 궁극기 게이지 (Ultimate Energy)

## 왜 필요했나
궁극기는 아무 때나 못 쓰게 하고, 지금 얼마나 모였는지 플레이어가 한눈에 알 수 있어야 합니다.

## 무엇을 설계했나
적중할 때마다 차오르고, **가득 찼을 때만 발동**되며, 상태를 아이콘으로 보여주는 게이지입니다.

## 어떻게 설계했나
`UltimateEnergy`를 GAS 어트리뷰트로 두고, 공격이 적중하는 경로에서 스킬 데이터의 `UltimateGain`만큼 적립합니다(변주 게이지와 동일한 방식). 발동은 궁극기 어빌리티(`GA_Liberation`)의 "게이지 풀" 조건(`Energy >= Max - 0.01f` — float 적립이 정확히 Max에 안 떨어지므로 여유값)으로 게이팅하고, 소모는 0으로 Override합니다(02 문서). HUD는 게이지 퍼센트로 궁 아이콘 뒤 라디얼을 채우고, 다 안 찼을 때는 아이콘을 회색 틴트로 눌러 "아직 못 쓴다"를 표현합니다.

적립은 적의 회피 판정(`TryDodge`) **뒤**에 있어, 회피당한 히트는 게이지를 한 톨도 못 올립니다(10 문서).

```cpp
// 적중 시 게이지 적립 (변주 게이지와 동일 패턴)
if (Skill->UltimateGain > 0.f)
    SourceASC->ApplyModToAttribute(UWuWa_AttributeSetBase::GetUltimateEnergyAttribute(),
                                   EGameplayModOp::Additive, Skill->UltimateGain);
```

**왜 GAS 어트리뷰트였나** — 캐릭터에 `float UltimateEnergy` 하나만 두는 게 더 간단해 보이지만, 그러면 (1) 0~Max 클램프를 손으로 해야 하고 (2) 값이 바뀔 때마다 HUD를 직접 찾아 갱신해줘야 합니다. 어트리뷰트로 두면 `PreAttributeChange`에서 클램프가 자동이고, 값 변경 델리게이트에 HUD가 바인딩만 해두면 UI가 알아서 따라옵니다 — HP·변주 게이지·궁극기 게이지가 전부 같은 규칙 하나로 굴러갑니다.

> 참고: 적립 로직은 `GA_BaseAttack::OnHitEvent`와 `GA_ResonanceSkill::OnHitEvent`에 **같은 패턴이 각각** 들어 있습니다(공유 함수 하나가 아니라 복제). 어빌리티 히트 처리를 하나로 합치는 것은 남은 리팩터링 지점입니다.
