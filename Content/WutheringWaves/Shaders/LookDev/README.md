# Rover Unlit LookDev

현재 비교본은 이 폴더의 `SK_Rover_ShadowStudy_Preview`입니다. 이 메시의 앞머리·머리카락·얼굴·상의·하의 슬롯은 각각 `MI_Rover_*_ShadowStudy`를 사용하며, 인스턴스의 부모는 `M_Rover_ShadowStudy_Unlit`입니다. 눈은 원래 `MI_Rover_Eye`를 사용합니다. 실제 게임에 사용하는 Rover 메시와 기존 `M_Character`는 이 미리보기에 연결하지 않았습니다.

이 재질은 **Unlit**입니다. Rover 원본 텍스처의 색상·법선·FTM/HM/HR 채널을 이용해 빛 방향에 따른 명암과 광택을 Emissive 안에서 계산합니다. 현재 보이는 그림자는 표면 방향을 따른 명암입니다. 얼굴의 방향별 그림자와 앞머리가 얼굴에 드리우는 그림자는 전용 마스크/투영 로직이 필요하며, 아직 완성된 기능으로 취급하면 안 됩니다. 옷의 겹침 그림자도 현재 기본 그림자 맵을 받아 계산하지 않습니다.

이전 시도와 텍스처 복제본 23개는 [`PreviousVersions`](./PreviousVersions)에 모았습니다. 원래 얼굴 재질을 쓴 비교본은 그 안의 `SK_Rover_SourceMaps_Preview`입니다. 이전 상세 기록은 `PreviousVersions/README_Legacy.md`에 있으며, 기록에 적힌 옛 에셋 경로는 이제 `PreviousVersions`가 추가된 경로로 읽어야 합니다.
