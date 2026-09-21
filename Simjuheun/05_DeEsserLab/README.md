# 05. DeEsserLab — Wide-band De-esser

치찰음 대역만 **찾은 뒤**, 치찰음이 강한 순간에는 **원본 신호 전체의 볼륨을 잠깐 줄이는** 학습용 JUCE 플러그인입니다.

## 이번 DSP의 핵심 구조

```text
입력 신호
├─ 감지 경로: Band-pass → Envelope Follower → Threshold / Ratio / Range
│                                      └─ 지금 얼마나 줄일지 계산
│
└─ 처리 경로: 원본 전체 × 계산된 Gain → 출력
```

Band-pass 필터는 소리를 직접 바꾸기 위한 필터가 아니라, `ㅅ`, `ㅆ`, `치`처럼 날카로운 고역이 얼마나 강한지 알아보기 위한 **센서**입니다. 센서가 Threshold를 넘으면 Compressor와 같은 방식으로 Gain Reduction을 계산하지만, Wide-band 방식이므로 고역만이 아니라 저역을 포함한 원본 전체에 같은 Gain을 곱합니다.

예를 들어 목소리에 7 kHz 치찰음과 300 Hz 몸통이 함께 있을 때 6 dB가 감쇄되면, 두 성분 모두 약 `0.5배`가 됩니다. 그래서 구현이 단순하고 자연스럽지만, 강하게 동작하면 치찰음이 나는 순간마다 목소리 전체가 살짝 꺼지는 느낌이 날 수 있습니다.

## 파라미터가 코드에서 하는 일

| 파라미터 | 역할 |
|---|---|
| Detector Frequency | 감지용 Band-pass의 중심 주파수 |
| Detector Q | 감지할 대역의 폭. 높을수록 더 좁게 감지 |
| Threshold | 감지 대역 레벨이 이 dBFS를 넘을 때 동작 시작 |
| Ratio | Threshold를 넘은 양 중 얼마를 줄일지 결정 |
| Range | 아무리 강한 치찰음이어도 줄일 수 있는 최대 dB |
| Attack | 치찰음을 발견한 뒤 감쇄가 걸리는 속도 |
| Release | 치찰음이 끝난 뒤 원래 볼륨으로 돌아오는 속도 |
| Detector Listen | 감지기가 듣고 있는 대역만 직접 청취 |

예를 들어 Detector가 `-12 dBFS`, Threshold가 `-24 dBFS`, Ratio가 `4:1`이면 Threshold를 12 dB 넘었습니다. 4:1은 그 12 dB를 출력에서는 3 dB만 남긴다는 뜻이므로, 필요한 Gain Reduction은 `12 - 3 = 9 dB`입니다. Range가 6 dB라면 실제 감쇄는 6 dB에서 멈춥니다.

## 왜 Makeup Gain이 없을까?

일반 Compressor는 전체 레벨을 계속 낮출 수 있어서 최종 음량을 보상하는 Makeup Gain이 유용합니다. De-esser는 치찰음이 튀는 짧은 순간만 교정하는 도구이므로, 자동으로 다시 키우면 줄였던 치찰음과 잡음까지 함께 올라올 수 있습니다. 그래서 이 학습용 버전에는 Makeup Gain을 넣지 않았습니다.

## 코드 읽는 순서

1. [`Source/DSP/Biquad.h`](Source/DSP/Biquad.h): 치찰음 감지용 Band-pass
2. [`Source/DSP/WideBandDeEsserDsp.h`](Source/DSP/WideBandDeEsserDsp.h): 감지, Envelope, Gain Reduction, 전체 신호 감쇄
3. [`Source/PluginProcessor.cpp`](Source/PluginProcessor.cpp): DAW 파라미터를 DSP에 전달하고 버퍼를 처리
4. [`Source/PluginEditor.cpp`](Source/PluginEditor.cpp): 노브, Detector 그래프, 레벨/GR 미터
5. [`Tests/DeEsserDspTests.cpp`](Tests/DeEsserDspTests.cpp): 7 kHz 감지와 Wide-band 동작 검증

## VS Code에서 확인

Command Palette에서 `Tasks: Run Task`를 열고 다음 순서로 실행합니다.

1. `1. Configure DeEsserLab`
2. `2. Build DSP Tests`
3. `3. Run DSP Tests`
4. `4. Build Plugin`

터미널에서는 다음과 같습니다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target DeEsserLabDspTests -j8
./build/DeEsserLabDspTests
cmake --build build --target DeEsserLab_All -j8
```

## Wide-band와 다음 단계

| 방식 | 감지하는 곳 | 실제로 줄이는 곳 |
|---|---|---|
| Wide-band | 치찰음 대역 | 원본 전체 |
| Split-band | 치찰음 대역 | 치찰음 대역만 |

이번 프로젝트에서는 두 열이 서로 다르다는 점이 가장 중요합니다. 다음 학습 단계에서는 원본을 저역과 치찰음 대역으로 나눈 뒤, 치찰음 대역만 줄이고 다시 합치는 Split-band 방식을 비교할 수 있습니다.
