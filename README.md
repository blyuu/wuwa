# Wuthering Waves — Combat Systems Recreation

원신·붕괴 스타일 액션 RPG **Wuthering Waves**의 전투 루프를 언리얼 엔진 5.5로 재구현한 개인 포트폴리오 프로젝트입니다. 평타 콤보부터 스킬·궁극기, 팀 편성/교체, 보스전(그로기·페이즈·AI), 타격감 보정까지 **상용 액션 게임의 전투 시스템을 GAS(Gameplay Ability System) 위에서 설계**하는 데 초점을 맞췄습니다.

각 시스템의 *왜 필요했나 → 무엇을 설계했나 → 어떻게 설계했나* 는 [설계 문서(`Docs/`)](Docs/README.md)에 정리되어 있습니다.

![Combat overview](Docs/media/hero.png)

---

## 목표 (Goals)

- 하나의 캐릭터가 아니라 **여러 캐릭터를 교체하며 싸우는 팀 기반 전투 루프**를 완성한다.
- 데미지·쿨타임·게이지·버프 같은 공통 규칙을 각 공격에 흩뿌리지 않고 **단일 지점으로 일원화**한다.
- 몹 사냥에서 끝나지 않고, **그로기·페이즈·연출**을 갖춘 보스전까지 하나의 흐름으로 연결한다.
- "맞은 것 같은데 안 맞는" 느낌을 줄이는 **타격 판정 보정과 타격감**까지 신경 쓴다.

## 핵심 기술 하이라이트 (Key Technical Highlights)

- **데미지 일원화** — `Damage`를 메타 어트리뷰트로 두고, 모든 데미지가 `AttributeSet::PostGameplayEffectExecute()` 한 곳을 통과하게 설계. 공격력 배수·그로기 취약(×1.5)·사망·데미지 넘버·타격음을 이 단일 초크 포인트에서 처리합니다.
- **GAS 기반 어빌리티** — 평타 콤보, 회피, 레조넌스 스킬, 궁극기(해방), 등장 인트로, 서포터 팀 버프를 각각 GameplayAbility로 분리하고 GameplayEffect로 쿨타임/버프를 표현.
- **팀 편성 & 캐릭터 교체** — 변주 게이지와 충전 교체를 포함한 팀 스왑 시스템. 교체 시 데미지도 동일한 데미지 파이프라인을 탑니다.
- **보스전** — HP 비율로 계산되는 페이즈, 평타 누적으로 발동하는 그로기(스턴 + 피해 ×1.5), 페이즈별로 사용 가능한 스킬만 골라 랜덤 재생하는 패턴 AI.
- **적 전투 AI** — Behavior Tree 기반 추격/공격에 더해, 공격 추적·확률 회피·포이즈를 조합한 반응형 전투.
- **타격감 보정** — 정밀 무기 스윕이 빗나가면, 정면 원뿔 + 사거리 게이트를 통과한 대상에 보정 히트를 넣는 2차 판정(소프트 락온 재사용).
- **전투 피드백** — 타격음, 스킬/피격 보이스, 플로팅 데미지 넘버를 `CombatFeedbackSubsystem`로 중앙 관리.

## 화면 (Media)

| | |
|---|---|
| ![](Docs/media/combat-01.png) | ![](Docs/media/combat-02.png) |
| ![](Docs/media/combat-03.png) | ![](Docs/media/combat-04.png) |

## 기술 스택 (Tech Stack)

- **Unreal Engine 5.5** / C++
- **Gameplay Ability System (GAS)** — 어빌리티·어트리뷰트·게임플레이 이펙트/태그
- **Behavior Tree / AIModule** — 적·보스 AI
- **Motion Warping · Animation Warping** — 공격 이동/정렬 보정
- **KawaiiPhysics** — 흔들림 물리
- **UMG** — HUD, 보스 체력바, 팀 초상화, 데미지 넘버

## 시스템 한눈에 보기 (Systems at a Glance)

| 시스템 | 핵심 클래스 | 설계 문서 |
|---|---|---|
| 데미지 파이프라인 | `WuWa_AttributeSetBase` | [01](Docs/01-damage-pipeline.md) |
| 레조넌스 스킬 & 궁극기 | `GA_ResonanceSkill`, `GA_Liberation` | [02](Docs/02-resonance-skill-ultimate.md) |
| 쿨타임 | `GE_CoolDown` | [03](Docs/03-cooldown.md) |
| 궁극기 게이지 | `WuWa_AttributeSetBase` | [04](Docs/04-ultimate-gauge.md) |
| 팀 편성 & 교체 | `TeamComponent` | [05](Docs/05-team-swap.md) |
| 서포터 버프 | `GA_TeamBuff`, `GE_AttackBuff` | [06](Docs/06-support-buff.md) |
| 플로팅 데미지 넘버 | `DamageNumberWidget` | [07](Docs/07-damage-numbers.md) |
| 보스 그로기 · 페이즈 · AI | `EnemyCharacter`, `WuwaEnemyController` | [08](Docs/08-boss-groggy-phase-ai.md) |
| 보스 등장 연출 | `GA_Intro` | [09](Docs/09-boss-intro-music.md) |
| 적 전투 AI 디테일 | `BTTask_EnemyAttack`, `BTService_UpdateTarget` | [10](Docs/10-enemy-combat-ai.md) |
| 전투 피드백 | `CombatFeedbackSubsystem` | [11](Docs/11-combat-feedback.md) |
| 히트 보정 (타격감) | `WeaponAnimNotifyState` | [12](Docs/12-hit-correction.md) |

## 프로젝트 구조 (Architecture)

기능 단위로 나눈 언리얼 표준 레이아웃을 따릅니다.

```
Source/WutheringWaves/
├─ Public / Private
│  ├─ Character/      BaseCharacter → PlayableCharacter / EnemyCharacter, Team·Equipment 컴포넌트
│  ├─ GameAbilities/  GAS 어빌리티(평타·회피·스킬·궁극·인트로·팀버프) + 어트리뷰트셋 + GE
│  ├─ Enemy/          Behavior Tree Task/Service, 스폰 박스·트리거, 적 컨트롤러
│  ├─ Weapon/         무기 클래스 / 데이터 애셋
│  ├─ AnimNotify/     히트 판정·태그 윈도우·타깃 추적 등 애님 노티파이
│  ├─ DataAsset/      캐릭터 / 적 데이터 애셋
│  ├─ Framework/      GameMode, CombatFeedbackSubsystem
│  ├─ UI/             HUD, 오버레이, 보스 체력바, 팀 초상화, 데미지 넘버
│  └─ GameplayTags/   프로젝트 게임플레이 태그 정의

Content/    언리얼 애셋(캐릭터·적·레벨·애니메이션 등)
Config/     프로젝트/입력 설정
Plugins/    KawaiiPhysics
Docs/       설계 문서 + media/
```

## 설계 문서 (Guided Reading)

전투의 핵심 흐름을 이해하려면 아래 순서를 추천합니다.

1. [데미지 일원화 — AttributeSet 단일 초크 포인트](Docs/01-damage-pipeline.md)
2. [레조넌스 스킬 & 궁극기](Docs/02-resonance-skill-ultimate.md)
3. [팀 편성 & 캐릭터 교체](Docs/05-team-swap.md)
4. [보스전 — 그로기 & 페이즈 & AI](Docs/08-boss-groggy-phase-ai.md)
5. [히트 보정 (타격감)](Docs/12-hit-correction.md)

전체 목록은 [`Docs/README.md`](Docs/README.md)에 있습니다.

## 빌드 & 실행 (Build & Run)

1. Unreal Engine **5.5**를 설치합니다.
2. `WutheringWaves.uproject`를 우클릭 → **Generate Visual Studio project files**.
3. `WutheringWaves.sln`을 열어 **Development Editor / Win64**로 빌드합니다.
4. 에디터에서 `Content/WutheringWaves/Level`의 전투 레벨을 열어 플레이합니다.

> 필수 플러그인(GAS, Motion/Animation Warping)은 `.uproject`에 명시되어 있어 별도 설정이 필요 없습니다.

## 현재 한계 & 로드맵 (Current Limitations)

- 싱글플레이(로컬) 전용 — 네트워크 리플리케이션은 범위 밖입니다.
- 콘텐츠 볼륨보다 **시스템 설계 검증**에 집중한 프로젝트입니다(캐릭터·보스 소수).
- 데미지 넘버 서브시스템 등 일부 항목은 설계는 확정, 구현은 진행 중입니다.
