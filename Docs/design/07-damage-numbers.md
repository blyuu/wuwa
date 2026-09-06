# 플로팅 데미지 넘버

## 왜 필요했나
"내가 얼마나 넣었는지"는 액션 게임의 핵심 피드백입니다. 다만 데미지 계산(게임플레이)과 숫자 UI를 직접 연결하면 결합도가 높아집니다.

## 무엇을 설계했나
적을 때린 위치 위에 **스크린 스페이스 숫자**가 떠오르고, 캐릭터 **속성 색**으로 표시되는 시스템입니다.

## 어떻게 설계했나
게임플레이와 UI를 `UCombatFeedbackSubsystem`(WorldSubsystem) 허브로 분리했습니다. 데미지 일원화 지점에서 (플레이어가 적을 때려 피해가 0보다 클 때) `ReportDamage(양·월드위치·속성태그)`를 브로드캐스트하면, `AWuwaHUD`가 이를 구독해 **월드 위치를 화면 좌표로 투영**하고 숫자 위젯을 스폰합니다. 화면 좌표에 약간의 랜덤 오프셋을 줘 여러 히트가 겹쳐도 숫자가 흩어져 보이게 하고, 위젯은 스스로 떠오르며 페이드된 뒤 제거됩니다. 색은 캐릭터 속성 태그로 정하고, 매핑이 없으면 흰색으로 폴백합니다.

```cpp
void AWuwaHUD::HandleDamageEvent(const FCombatFeedbackEvent& E)
{
    FVector2D Screen;
    if (!PC->ProjectWorldLocationToScreen(E.WorldLocation, Screen)) return;  // 화면 밖이면 무시
    Screen.X += FMath::FRandRange(-Scatter, Scatter);   // 겹침 방지 랜덤 오프셋
    Screen.Y += FMath::FRandRange(-Scatter, Scatter);
    UDamageNumberWidget* W = CreateWidget<UDamageNumberWidget>(PC, DamageNumberClass);
    W->SetDamage(E.Amount, E.ElementTag);   // 속성 -> 색 (미매핑 시 흰색)
    W->AddToViewport();
    W->SetPositionInViewport(Screen, true);
}
```

**왜 서브시스템 허브였나** — `AttributeSet`가 위젯을 직접 만들면 게임플레이 로직이 UI 클래스에 묶여, UI를 바꿀 때마다 전투 코드를 건드려야 합니다. 중간에 이벤트 허브를 두면 게임플레이는 "얼마·어디·무슨 속성"만 방송하고, 그 표현(스크린 스페이스 숫자든 다른 무엇이든)은 HUD가 독립적으로 결정합니다.

> 참고: 숫자는 **플레이어가 적을 때렸을 때만** 뜹니다(적→플레이어 피해는 제외). 그리고 현재는 히트마다 위젯을 생성/파괴합니다 — 위젯 **풀링은 아직 도입 전**으로, 스크린 스페이스 구조가 풀링에 유리하다는 점을 감안한 다음 최적화 대상입니다.
