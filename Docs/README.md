# Project Wuthering Waves — 문서

전투 시스템 설계 기록과 개발 로그를 모아 둔 폴더입니다.

```
Docs/
├─ design/   시스템별 설계 문서 (왜 필요했나 → 무엇을 → 어떻게)
├─ devlog/   특정 주제 딥다이브 · 리팩터링 기록
├─ drafts/   포트폴리오 초안 (Notion 정리본으로 대체됨)
└─ media/    스크린샷 · GIF · 다이어그램
```

## 설계 문서 (`design/`)

각 문서는 *왜 필요했나 → 무엇을 설계했나 → 어떻게 설계했나* 순서로 구성되어 있습니다.

1. [데미지 일원화 — AttributeSet 단일 초크 포인트](design/01-damage-pipeline.md)
2. [레조넌스 스킬 & 궁극기](design/02-resonance-skill-ultimate.md)
3. [쿨타임 시스템](design/03-cooldown.md)
4. [궁극기 게이지](design/04-ultimate-gauge.md)
5. [팀 편성 & 캐릭터 교체 (변주 게이지 · 충전 교체)](design/05-team-swap.md)
6. [버프 서포터 캐릭터 (팀 공격력 버프)](design/06-support-buff.md)
7. [플로팅 데미지 넘버](design/07-damage-numbers.md)
8. [보스전 — 그로기 & 페이즈 & AI](design/08-boss-groggy-phase-ai.md)
9. [보스 등장 연출 (스폰 인트로 + 음악)](design/09-boss-intro-music.md)
10. [적 전투 AI 디테일 (공격 추적 · 확률 회피 · 포이즈)](design/10-enemy-combat-ai.md)
11. [전투 피드백 (타격음 · 스킬/피격 보이스)](design/11-combat-feedback.md)
12. [히트 보정 (타격감)](design/12-hit-correction.md)

## 개발 로그 (`devlog/`)

- [히트 판정 설계 딥다이브](devlog/2026-08-12_HitDetection.md)
- [캐릭터 클래스 계층 리팩터링](devlog/2026-08-26_CharacterHierarchyRefactor.md)
- [전투 파이프라인 딥다이브](devlog/DeepDive_CombatPipeline.md)
