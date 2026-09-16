# JUCE Project Lab

JUCE 오디오 플러그인을 작은 단위로 직접 구현하고 검증하기 위한 학습 저장소입니다.
완성 제품보다는 **한 프로젝트에서 한 가지 개념을 명확히 배우는 것**을 목표로 합니다.

## 프로젝트

| 폴더 | 목적 | 방식 |
|---|---|---|
| `SimpleGainPlugin/` | Gain, 파라미터, UI 연결, DSP 테스트 | CMake · JUCE 8 |
| `EQPlugin/` | 기존 EQ 실습 자료 | Projucer |
| `VoltaAgentPlugin/` | 기존 플러그인·서버 연동 실습 자료 | Projucer |

`Honenuki*`, `VoltaDeEsserPlugin`, `Vota_JUCE`는 로컬에 함께 있을 수 있지만 각각의
별도 저장소로 관리하며 이 학습 저장소에는 포함하지 않습니다.

## 새 실습 플러그인 규칙

```text
PluginName/
├── CMakeLists.txt       # 빌드 설정의 원본
├── README.md            # 학습 목표와 실행 방법
├── Source/
│   ├── DSP/             # DAW/JUCE와 분리해 테스트 가능한 알고리즘
│   ├── PluginProcessor.*
│   └── PluginEditor.*
└── Tests/               # 작은 입력과 기대 출력으로 DSP 검증
```

- 플러그인마다 고유한 번들 ID와 제조사/플러그인 코드를 사용합니다.
- DSP는 가능한 한 `Source/DSP`에 분리하고, `processBlock()`은 연결 역할에 집중합니다.
- `build/`, `Builds/`, `JuceLibraryCode/`, AU/VST3 바이너리는 커밋하지 않습니다.
- CMake를 빌드 설정의 원본으로 사용하고 생성된 Xcode 프로젝트는 Git에서 제외합니다.
- 기능을 추가할 때 작은 자동 테스트와 DAW 확인 절차를 함께 남깁니다.

## 기본 환경

- JUCE 8.0.12
- CMake 3.22 이상
- C++17
- macOS에서는 Xcode Command Line Tools

각 플러그인은 로컬 JUCE 체크아웃을 우선 사용하고, 없으면 지정된 JUCE 버전을
CMake가 내려받도록 구성할 수 있습니다. 구체적인 명령은 각 프로젝트 README를
참고하세요.
