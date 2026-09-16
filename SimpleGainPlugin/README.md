# GainLab

Gain 하나를 통해 JUCE 플러그인의 전체 흐름을 배우는 첫 번째 실습 프로젝트입니다.

```text
UI 노브 → JUCE 파라미터 → processBlock() → Gain DSP → 출력 샘플
```

## 현재 구현

- AU, VST3, Standalone
- 모노·스테레오 입력/출력
- Gain 범위 `-60 dB ~ +12 dB`
- UI 노브와 DAW 파라미터 연결
- 파라미터 저장·복원
- JUCE와 분리된 순수 Gain DSP
- DAW 없이 실행하는 DSP 자동 테스트

Gain 변화 스무딩은 아직 구현하지 않았습니다. 빠른 파라미터 변화에서 발생할 수 있는
클릭·지퍼 노이즈를 확인한 뒤 다음 학습 단계에서 추가합니다.

## 코드 구조

```text
Source/DSP/GainDsp.h       dB 변환과 샘플 곱셈
Source/PluginProcessor.*   DAW 버퍼·파라미터와 DSP 연결
Source/PluginEditor.*      플러그인 화면과 Gain 노브
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
PASS: 모든 Gain DSP 테스트를 통과했습니다.
```

## macOS 개발용 설치 위치

```text
AU    ~/Library/Audio/Plug-Ins/Components/
VST3  ~/Library/Audio/Plug-Ins/VST3/
```

일반 배포에는 개발용 ad-hoc 서명이 아니라 정식 코드 서명과 공증이 필요합니다.
