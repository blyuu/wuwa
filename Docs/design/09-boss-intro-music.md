# 보스 등장 연출 (스폰 인트로 + 보스 음악)

## 왜 필요했나
보스는 등장부터 무게감이 있어야 합니다. 스폰되자마자 바로 달려들면 연출이 죽습니다.

## 무엇을 설계했나
스폰 시 **플레이어를 바라보며 인트로 몽타주 + 대사**를 재생하고, 그 연출이 끝난 뒤 전투를 시작합니다. 보스는 등장부터 처치까지 **전용 배틀 음악**이 흐릅니다.

## 어떻게 설계했나
스폰 시 `IntroMontage`가 설정돼 있으면 인트로를 재생합니다 — "보스 전용"이 아니라 **데이터에 인트로가 있으면 누구나** 연출이 붙고, 배틀 음악만 `bIsBoss`로 게이팅합니다. 인트로는 플레이어 쪽으로 회전한 뒤 몽타주와 대사를 재생하고, 그동안 AI를 `PauseLogic`으로 멈췄다가 몽타주가 끝나는 순간 `ResumeLogic`으로 전투를 시작합니다.

두 가지 안전장치를 뒀습니다.
- **재생 실패 대비** — `Montage_Play`가 돌려준 길이가 0 이하면(몽타주 미설정 등) 인트로를 건너뛰고 AI를 멈추지 않습니다. 연출이 없는 적이 영원히 정지하는 사고를 막습니다.
- **늦은 빙의 대비** — 배치된 보스는 `PlayIntro`가 직접 pause하지만, 런타임에 늦게 빙의되는 적은 `WuwaEnemyController::OnPossess`가 `IsPlayingIntro()`를 보고 다시 pause합니다. 두 경로 모두 커버됩니다.

음악은 데이터 에셋의 루프 사운드를 `SpawnSound2D`로 재생해 컴포넌트(`BattleMusicComp`)로 들고 있다가, 처치 시 `FadeOut`으로 페이드합니다.

```cpp
void AEnemyCharacter::PlayIntro()
{
    // 플레이어 바라보게 회전 (생략) …

    const float PlayLen = GetMesh()->GetAnimInstance()->Montage_Play(EnemyDataAsset->IntroMontage);
    if (PlayLen <= 0.f) return;                 // 재생 실패 → 전투를 막지 않음
    bIntroPlaying = true;

    if (AAIController* AI = Cast<AAIController>(GetController()))
        if (UBrainComponent* Brain = AI->GetBrainComponent())
            Brain->PauseLogic(TEXT("Intro"));
    // 몽타주 끝나면 OnIntroMontageEnded → ResumeLogic 으로 전투 시작
}
```

**왜 빙의 타이밍까지 처리했나** — "BeginPlay에서 pause"만 하면 컨트롤러가 아직 안 붙은(늦게 빙의되는) 스폰 보스에서 pause가 씹혀 인트로 도중에 움직여 버립니다. 직접 경로 + 컨트롤러 `OnPossess` 경로를 둘 다 두어, 배치 보스와 런타임 스폰 보스 어느 쪽이든 연출이 끝날 때까지 멈춰 있게 했습니다.
