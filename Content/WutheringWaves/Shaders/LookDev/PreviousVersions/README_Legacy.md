# Rover Unlit Toon LookDev

첫 번째 참고 영상의 셀 셰이딩 방식을 바탕으로 `M_Character_UnlitToon`을 새로 만들었습니다. `M_Character`처럼 Unlit이고, 캐릭터 색상 텍스처를 바탕으로 그림자 경계, 빛 방향에 따른 하이라이트, 부드러운 림 라이트를 계산해 Emissive에 출력합니다. 주광 방향은 SkyAtmosphere의 태양광을 사용합니다. Unlit에서는 Normal 출력이 조명에 반영되지 않으므로 법선 텍스처를 머티리얼 내부에서 월드 방향으로 변환해 사용합니다.

`M_Character_FaceSDF`에는 빛 방향과 머리 방향에 따른 얼굴 SDF 그림자, 앞머리 그림자 마스크 입력을 추가했습니다. Rover 원본 텍스처에서 전용 얼굴 SDF와 앞머리 그림자 마스크는 확인되지 않았으므로 `MI_Rover_Face_UnlitToon`의 `FaceSDFStrength`, `BangsShadowStrength` 기본값은 `0`입니다. 해당 텍스처를 제작한 뒤 인스턴스의 `FaceSDFTexture`, `BangsShadowTexture`에 지정하고 강도를 올리면 됩니다. 머리의 로컬 +X를 정면, +Y를 오른쪽으로 가정했습니다. 모델 축이 다르면 `HeadForwardLocal`, `HeadRightLocal`을 조정해야 합니다.

원본 텍스처 연결을 확인하는 기준 에셋은 `SK_Rover_SourceMaps_Preview`입니다. Rover 메시를 복제해 앞머리·머리카락·상의·하의에는 `MI_Rover_*_SourceMaps`를 연결하고, 얼굴과 눈에는 원래 `MI_Rover_Face`, `MI_Rover_Eye`를 사용합니다. 실제 게임에 사용하는 Rover 메시와 기존 `M_Character`는 변경하지 않았습니다. 이전 `SK_Rover_SoftToon_Preview`, `SK_Rover_UnlitToon_Preview`는 초기 설정 비교용입니다.

그림자 강도를 비교하는 최신 에셋은 `SK_Rover_ShadowStudy_Preview`입니다. `M_Rover_ShadowStudy_Unlit`은 같은 원본 텍스처를 쓰면서 머리카락·옷의 그림자 색을 더 어둡게 하고 명암 전환 폭을 줄였습니다. SkyAtmosphere 태양광이 없는 미리보기 씬에서는 고정 방향광으로 대체해 법선 명암이 완전히 사라지지 않도록 했습니다. 얼굴 슬롯에도 `MI_Rover_Face_ShadowStudy`를 적용해 방향광 명암을 비교할 수 있습니다. 기존 얼굴 머티리얼은 수정하지 않았으므로 얼굴 원본을 보려면 `SK_Rover_SourceMaps_Preview`를 열면 됩니다.

두 머티리얼 모두 Unlit이므로 이 명암은 법선과 빛 방향으로 만든 것입니다. **앞머리·옷·팔이 다른 부위에 실제로 드리우는 그림자는 받지 않습니다.** 그런 그림자가 필요하면 Lit 기반 별도 머티리얼 또는 전용 그림자 마스크/투영 기법이 필요합니다.

비교하려면 Skeletal Mesh Editor에서 `SK_Rover_SourceMaps_Preview`를 열고 SkyAtmosphere 태양광이 있는 미리보기 씬을 사용하세요. 새 `M_Rover_SourceMaps_Unlit`은 원본 텍스처를 직접 참조합니다. 앞머리·머리카락은 `_HN.rg`로 법선을 복원하고 `_HM`의 영역·이동값과 `_HR`의 램프로 약한 광택을 만듭니다. 옷은 `_N.rg`로 법선을 복원하고 `_N.a`로 광택 폭을, `_FTM.g`로 금속성 영역의 광택 강도를 조절합니다. 머리카락에는 `_FTM.r`도 광택 가시성에 약하게 반영합니다. Rover에는 `Bangs_HR`이 없어서 앞머리에도 `Hair_HR`을 공유합니다. 이 매핑은 원본 게임 셰이더를 완전히 재현한 것이 아니라 해당 채널의 역할을 참고한 Unlit 근사 구현입니다.

원본 데이터 텍스처의 가져오기 설정은 유지했습니다. `_HN`, `_N`, `_FTM`, `_HM`은 원본에서 sRGB 텍스처로 저장돼 있어 새 머티리얼 내부에서 사용 채널을 역변환해 읽습니다. 기존 `T_Rover_*_Normal` 복제 텍스처는 새 `SourceMaps` 인스턴스에서 **사용하지 않습니다**. 이전 미리보기용 인스턴스가 참조하므로 아직 LookDev 폴더에 남아 있습니다.

영상의 inverted-hull 외곽선은 **아직 포함되지 않았습니다**. UE 5.5 Python 머티리얼 그래프 API에서 외곽선 메시 확장에 필요한 World Position Offset 출력이 숨겨져 있어 자동 연결할 수 없었습니다. 따라서 현재 미리보기에서는 그림자 경계, 하이라이트, 림 라이트, 얼굴용 입력을 확인할 수 있지만 외곽선은 보이지 않습니다. 앞머리 그림자 역시 마스크가 공급되기 전까지 비활성 상태입니다.

## Rover 원본 텍스처 확인

`/Game/WutheringWaves/Character/Models/Rover/textures`의 `T_R2T1PlayerFemaleMd20011*` 텍스처를 확인했습니다. 새 `SourceMaps` 미리보기는 각 파츠의 `_D`, 앞머리·머리카락의 `_HN`과 `_FTM`·`_HM`, 상의·하의의 `_N`과 `_FTM`, 머리카락의 `_HR`을 모두 원본 경로에서 직접 사용합니다.

`_FTM`은 앞머리·머리카락·상의·하의에 모두 있고, `_HM`은 앞머리·머리카락에, `_HR`은 머리카락에, `_RGID`는 상의·하의에 있습니다. `_RGID`는 영역 ID를 담은 데이터라 정확한 디코딩과 영역별 파라미터가 필요한데, 현재 단순화된 LookDev에는 연결하지 않았습니다. `_FTM`의 모든 채널이나 원본 셰이더의 모든 머리카락 밴드도 아직 재현하지 않았습니다.

채널 용도는 [Wuthering Waves 프레임 분석](https://ferina.top/index.php/2026/07/03/wuwa-frame-analyze/)과 Rover 텍스처의 채널 분포를 함께 참고했습니다. 프레임 분석의 대상 캐릭터와 Rover가 같지는 않으므로, 여기의 연결은 미리보기용 근사 구현으로 취급합니다.
