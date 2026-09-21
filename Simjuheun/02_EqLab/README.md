# EqLab

FIR과 IIR의 차이를 직접 코드와 소리로 확인하며 EQ를 만드는 두 번째 JUCE 실습입니다.

## FIR과 IIR

EQ는 특정 주파수 성분을 키우거나 줄이는 필터입니다. FIR과 IIR은 그 필터의 출력을
계산하는 서로 다른 방식입니다.

```text
FIR: 현재 입력 + 과거 입력들 → 출력
IIR: 현재·과거 입력 + 과거 출력 → 출력
```

- **FIR(Finite Impulse Response)**: 과거 입력을 정해진 개수만 사용하므로 impulse에 대한
  반응이 유한합니다. 많은 tap이 필요할 수 있지만 선형 위상 필터를 만들 수 있습니다.
- **IIR(Infinite Impulse Response)**: 과거 출력을 feedback하므로 적은 계산으로 급한
  필터를 만들 수 있습니다. 일반적인 실시간 파라메트릭 EQ에서 많이 사용합니다.
- **Biquad**: 과거 입력 2개와 과거 출력 2개를 사용하는 2차 IIR 필터입니다. 여러 개를
  연결하면 Low Cut, High Cut, Bell, Shelf 같은 EQ 밴드를 구성할 수 있습니다.

## 현재 단계

현재 하나의 Biquad가 선택한 필터 공식에 따라 Bell, Pass, Shelf, Notch로 동작합니다.

- Filter Type: Bell / Low-pass / High-pass / Low-shelf / High-shelf / Notch 선택
- Frequency: Bell과 Shelf의 중심 또는 Pass 필터의 Cutoff
- Gain: Bell의 변화량 또는 Shelf가 도달할 저역·고역 Gain
- Q: Bell의 폭 또는 Cutoff 주변의 공진 정도
- Shelf Slope: 0 dB 영역과 설정 Gain 영역을 연결하는 경사
- Notch: Frequency를 상쇄하고 Q로 제거 영역의 폭을 조절

필터 종류가 바뀌어도 샘플 처리식은 동일합니다. 선택한 종류에 맞는 공식으로 `b0`,
`b1`, `b2`, `a1`, `a2`를 다시 계산합니다. GUI의 응답 곡선도 같은 계수를 사용하므로
현재 설정이 각 주파수를 얼마나 바꾸는지 바로 확인할 수 있습니다.

## 구조

```text
Source/DSP/BiquadDsp.h      여섯 필터의 계수 계산, 응답 계산, 샘플 처리
Source/PluginProcessor.*   DAW 오디오 버퍼와 DSP 연결
Source/PluginEditor.*      필터 선택, 노브, 실시간 응답 곡선 UI
Tests/EqLabDspTests.cpp    Bell, Pass, Shelf, Notch의 주파수 응답 검증
```

## 빌드와 테스트

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
./build/EqLabDspTests
ctest --test-dir build --output-on-failure
```

VS Code에서는 `Tasks: Run Test Task`에서 `EqLab: Run DSP Test`를 선택할 수 있습니다.
