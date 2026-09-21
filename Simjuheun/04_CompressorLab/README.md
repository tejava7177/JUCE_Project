# 04 CompressorLab

전체 신호의 레벨을 측정하고 큰 부분만 자동으로 줄이는 기본 Peak Compressor를
직접 구현하는 학습 프로젝트입니다.

## 이것이 Compressor DSP다

```text
입력 샘플
├─ Peak 측정 → Envelope Follower → Threshold/Ratio → Gain Reduction
└─ 원본 × Gain Reduction 배율 → Makeup Gain → 출력
```

DSP는 단순히 샘플에 Gain을 곱하는 한 줄만 뜻하지 않습니다. 레벨을 측정하고,
얼마나 줄일지 판단하고, 시간 변화를 부드럽게 만들고, 최종 배율을 적용하는
전체 디지털 신호 처리 과정입니다.

## 파라미터

| 파라미터 | 역할 |
|---|---|
| Threshold | Compression이 시작되는 dBFS 기준점 |
| Ratio | Threshold 초과분을 얼마나 억제할지 |
| Attack | 큰 소리에 Gain Reduction이 따라가는 속도 |
| Release | 소리가 작아진 뒤 Gain이 복구되는 속도 |
| Makeup Gain | Compression으로 내려간 전체 청취 레벨 보상 |

## Gain Reduction 미터

GR 미터는 출력 볼륨이 아니라 컴프레서가 현재 줄이고 있는 양을 표시합니다.

```text
0 dB  = 압축하지 않음
-3 dB = 약 0.708배
-6 dB = 약 0.5배
```

## 코드에서 볼 순서

1. `Source/DSP/CompressorDsp.h` — 전체 Compressor DSP
2. `Tests/CompressorDspTests.cpp` — Ratio, Makeup, Stereo Link 확인
3. `Source/PluginProcessor.cpp` — DAW 버퍼와 연결
4. `Source/PluginEditor.cpp` — Transfer Curve와 GR 미터

## VS Code에서 테스트

`Terminal → Run Task`에서 다음 순서로 실행합니다.

1. `1. Configure CompressorLab`
2. `3. Run DSP Tests`
3. `4. Build Plugin`
