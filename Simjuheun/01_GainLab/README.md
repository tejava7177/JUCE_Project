# GainLab

Gain에서 시작해 기본 DSP를 하나씩 연결하며 JUCE 플러그인의 전체 흐름을 배우는
첫 번째 실습 프로젝트입니다.

```text
입력 → Gain → Delay 버퍼 → Wet
  └──────────────────────→ Dry
              Dry/Wet 혼합 → 출력
```

## 현재 구현

- AU, VST3, Standalone
- 모노·스테레오 입력/출력
- Gain 범위 `-60 dB ~ +12 dB`
- Delay Time 범위 `1 ms ~ 1000 ms`
- Feedback 범위 `0% ~ 95%`
- Dry/Wet 범위 `0% ~ 100%`
- UI 노브와 DAW 파라미터 연결
- 파라미터 저장·복원
- Gain, Delay Time, Feedback, Mix에 20 ms 선형 smoothing
- JUCE와 분리된 순수 Gain·Delay·Dry/Wet DSP
- DAW 없이 실행하는 DSP 자동 테스트

Delay는 원형 버퍼에 현재 샘플을 저장하고 `Delay Time`만큼 과거의 샘플을 읽습니다.
`Feedback`은 읽은 샘플 일부를 버퍼에 다시 넣어 반복음을 만듭니다. Dry/Wet은
`Output = Dry × (1 - Mix) + Wet × Mix`로 계산합니다. Send/Aux에서 사용할 때는 원본
트랙이 이미 Dry를 재생하므로 Mix를 `100% Wet`으로 두는 것이 기본입니다.

Delay 버퍼와 smoother는 Processor 멤버로 유지되어 오디오 블록이 바뀌어도 과거 샘플과
진행 상태를 이어갑니다. 재생 중 메모리 할당을 피하기 위해 Delay 버퍼는
`prepareToPlay()`에서 미리 준비합니다.

## 코드 구조

```text
Source/DSP/DelayDsp.h       원형 버퍼, 시간 지연, feedback
Source/DSP/DryWetDsp.h      원본과 처리 결과 혼합
Source/DSP/GainDsp.h       dB 변환과 샘플 곱셈
Source/DSP/LinearSmoother.h 블록 사이에 유지되는 선형 ramp
Source/PluginProcessor.*   DAW 버퍼·파라미터와 DSP 연결
Source/PluginEditor.*      플러그인 화면과 네 개의 노브
Tests/GainDspTests.cpp     알려진 샘플 배열로 DSP 검증
```

## 빌드

프로젝트 폴더에서 실행합니다.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
```

로컬 JUCE 위치가 기본값과 다르면 다음처럼 지정할 수 있습니다.

```bash
cmake -S . -B build -DGAINLAB_JUCE_PATH=/path/to/JUCE
```

## DSP 테스트

```bash
cmake --build build --target GainLabDspTests
./build/GainLabDspTests
ctest --test-dir build --output-on-failure
```

VS Code에서는 `Tasks: Run Test Task`에서 `GainLab: Run DSP Test`를 선택할 수 있습니다.

기대 출력:

```text
입력 샘플: [ 1, 0.5, -0.5, -1 ]
Gain: -6.0206 dB -> 0.5배
출력 샘플: [ 0.5, 0.25, -0.25, -0.5 ]
Dry/Wet 0%, 50%, 100%: [ 0.8, 0.6, 0.4 ]
Delay impulse: [ 0, 0, 0, 1, 0, 0, 0.5 ]
Smoothing block 1: [ 0.875, 0.75 ]
Smoothing block 2: [ 0.625, 0.5 ]
PASS: Gain, Dry/Wet, Delay, smoothing 테스트를 모두 통과했습니다.
```

## macOS 개발용 설치 위치

```text
AU    ~/Library/Audio/Plug-Ins/Components/
VST3  ~/Library/Audio/Plug-Ins/VST3/
```

일반 배포에는 개발용 ad-hoc 서명이 아니라 정식 코드 서명과 공증이 필요합니다.
