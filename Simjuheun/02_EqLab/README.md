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

현재 하나의 IIR Biquad Bell EQ가 실제 오디오 처리에 연결되어 있습니다.

- Frequency: 어느 주파수를 처리할지
- Gain: 해당 주파수를 얼마나 키우거나 줄일지
- Q: 처리할 주파수 범위를 얼마나 좁거나 넓게 할지

플러그인은 세 파라미터와 Sample Rate를 Bell 공식에 넣어 `b0`, `b1`, `b2`, `a1`,
`a2` 계수를 계산합니다. 좌우 채널은 각자 과거 입력과 출력을 기억하며, Gain 0 dB에서는
입력을 그대로 출력합니다.

## 구조

```text
Source/DSP/BiquadBellDsp.h  Bell 계수 계산과 샘플 처리
Source/PluginProcessor.*   DAW 오디오 버퍼와 DSP 연결
Source/PluginEditor.*      Frequency, Gain, Q UI
Tests/EqLabDspTests.cpp    1 kHz에서 실제 +6 dB인지 검증
```

## 빌드와 테스트

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
./build/EqLabDspTests
ctest --test-dir build --output-on-failure
```

VS Code에서는 `Tasks: Run Test Task`에서 `EqLab: Run DSP Test`를 선택할 수 있습니다.
