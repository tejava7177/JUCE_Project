# 03 DynamicEqLab

고정된 Bell EQ에 **대역 감지기와 시간에 따른 Gain 제어**를 붙여,
하향식(Downward) Dynamic EQ를 직접 구현하는 학습 프로젝트입니다.

## 일반 EQ와 무엇이 다른가?

일반 Bell EQ는 `Frequency`, `Q`, `Gain`을 정하면 Gain이 계속 유지됩니다.
Dynamic EQ는 선택한 대역의 에너지를 측정하고, Threshold를 넘는 순간에만
Bell의 Gain을 음수 방향으로 움직입니다.

```text
입력
 ├─ 감지 경로: Band-pass → 절댓값 → Envelope → Threshold/Ratio
 └─ 처리 경로: 계산된 Gain으로 Bell 계수 갱신 → 출력
```

필터 공식은 EqLab에서 사용한 Biquad Bell과 같습니다. 새로 추가된 핵심은
`Detector`, `Threshold`, `Ratio`, `Range`, `Attack`, `Release`입니다.

## 파라미터

| 파라미터 | 역할 |
|---|---|
| Frequency | 감지하고 처리할 중심 주파수 |
| Q | 감지 및 처리할 대역의 폭 |
| Threshold | 동적 감쇄가 시작되는 대역 레벨 |
| Ratio | Threshold를 넘은 양을 얼마나 강하게 억제할지 |
| Range | 한 번에 줄일 수 있는 최대 dB |
| Attack | 문제가 커졌을 때 감쇄가 따라가는 속도 |
| Release | 문제가 사라진 뒤 원래 상태로 돌아오는 속도 |

예를 들어 Detector가 `-12 dBFS`, Threshold가 `-24 dBFS`, Ratio가 `4:1`이면
Threshold를 12 dB 넘었습니다.

```text
감쇄량 = 초과량 × (1 - 1 / Ratio)
       = 12 × (1 - 1/4)
       = 9 dB
```

Range가 6 dB라면 최종 감쇄량은 6 dB로 제한됩니다.

## 코드에서 볼 순서

1. `Source/DSP/Biquad.h` — Band-pass와 Bell 필터
2. `Source/DSP/DynamicBellDsp.h` — 감지, Threshold 판정, Attack/Release, 동적 Gain
3. `Source/PluginProcessor.cpp` — DAW 버퍼와 DSP 연결
4. `Source/PluginEditor.cpp` — 노브와 현재 감지량/감쇄량 표시
5. `Tests/DynamicEqDspTests.cpp` — DAW 없이 수식과 오디오 동작 확인

## VS Code에서 테스트

`Terminal → Run Task`에서 다음 순서로 실행합니다.

1. `1. Configure DynamicEqLab`
2. `3. Run DSP Tests`
3. `4. Build Plugin`

빌드 결과는 `build/DynamicEqLab_artefacts/Debug/` 아래에 생성됩니다.
