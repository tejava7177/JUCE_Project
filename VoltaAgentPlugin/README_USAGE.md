# VoltaAgentPlugin Usage Guide

이 문서는 `VoltaAgentPlugin` 레포지토리를 처음 보는 팀원을 위한 **실행/사용 가이드**다.  
JUCE를 다뤄본 적이 없어도 따라갈 수 있도록, 프로젝트 구조와 현재 동작 방식까지 같이 설명한다.

## 1. 이 프로젝트가 하는 일

`VoltaAgentPlugin`은 일반적인 오디오 이펙터 플러그인이 아니라, 다음 역할을 하는 **채팅형 믹싱 어시스턴트 클라이언트**다.

- 사용자가 DAW 안에서 AI와 대화할 수 있는 UI 제공
- 서버에 stem WAV를 업로드해서 분석 실행
- 서버의 분석 결과와 채팅 응답 표시
- 향후 Ableton 세션 제어/적용 흐름과 연결될 중앙 패널 역할

현재 기준으로 이 플러그인이 잘하는 것은 두 가지다.

1. `stem WAV 업로드 + 분석 결과 확인`
2. `프로젝트 채팅 인터페이스 사용`

반대로 아직 완전히 되지 않은 것은 다음이다.

- 채팅 결과를 실제 Ableton 트랙/버스/파라미터에 자동 적용
- `/projects` 채팅 응답을 machine-readable action으로 바꾸고 apply와 연결

## 2. JUCE를 모르는 사람을 위한 아주 짧은 배경

JUCE는 오디오 플러그인과 오디오 앱을 만들 때 많이 쓰는 C++ 프레임워크다.

이 레포에서 JUCE와 관련해 알아야 할 최소 정보만 정리하면:

- [VoltaAgentPlugin.jucer](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/VoltaAgentPlugin.jucer)
  - JUCE 프로젝트 설정 파일
- [Builds/MacOSX](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Builds/MacOSX)
  - Xcode 프로젝트가 있는 폴더
- [Source](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source)
  - 실제 C++ 소스 코드

JUCE 플러그인의 기본 구조는 보통 이렇게 나뉜다.

- `PluginProcessor`
  - 상태 관리
  - 서버 요청
  - 플러그인 로직
- `PluginEditor`
  - 실제 화면 레이아웃
- `UI/*`
  - 화면 컴포넌트

즉 이 프로젝트에서 화면을 보고 싶으면 `PluginEditor`와 `UI` 폴더를 보면 되고, 서버 연동을 보고 싶으면 `PluginProcessor`와 `Communication/SessionControlClient.h`를 보면 된다.

## 3. 현재 프로젝트 구조

중요한 파일만 먼저 보면 된다.

- [Source/PluginProcessor.h](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/PluginProcessor.h)
- [Source/PluginProcessor.cpp](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/PluginProcessor.cpp)
- [Source/PluginEditor.cpp](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/PluginEditor.cpp)
- [Source/UI/ControllerView.h](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/UI/ControllerView.h)
- [Source/UI/ControllerView.cpp](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/UI/ControllerView.cpp)
- [Source/UI/DebugPanel.h](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/UI/DebugPanel.h)
- [Source/UI/DebugPanel.cpp](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/UI/DebugPanel.cpp)
- [Source/Communication/SessionControlClient.h](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/Communication/SessionControlClient.h)

역할은 대략 이렇게 보면 된다.

- `PluginProcessor`
  - 서버 endpoint 관리
  - stem folder 상태 관리
  - `/projects` API 호출
  - activity log, explanation, preview 텍스트 상태 보관
- `SessionControlClient`
  - 실제 HTTP 요청 수행
  - JSON 응답 파싱
- `ControllerView`
  - 사용자가 보는 메인 채팅 UI
- `DebugPanel`
  - 개발용 상세 정보 UI

## 4. 현재 서버와 연결되는 방식

현재 `VoltaAgentPlugin`은 `mixing-extension` 서버의 `/projects/*` 축을 주로 사용한다.

대표 API는 다음과 같다.

- `POST /projects`
- `POST /projects/<id>/upload`
- `GET /projects/<id>/analysis`
- `POST /projects/<id>/chat`

쉽게 말해 현재 흐름은:

1. 프로젝트 세션 생성
2. stem WAV 업로드
3. 분석 결과 조회
4. 채팅 요청 전송

순서다.

## 5. 사전 준비

### 필수 준비물

1. `mixing-extension` 서버 코드
2. Python 가상환경과 서버 실행 가능 상태
3. Ableton Live
4. `VoltaAgentPlugin` 빌드 결과
5. 분석할 stem WAV 폴더

### stem WAV란?

여기서 말하는 stem WAV는 보통 이런 파일들이다.

- `Kick.wav`
- `Bass.wav`
- `Lead Vocal.wav`
- `Guitar_Intro.wav`

즉 MIDI나 프로젝트 파일이 아니라, **실제 렌더링된 오디오 파일**이다.

현재는 다음을 전제로 한다.

- 입력 포맷: `WAV`
- 입력 단위: `트랙별 stem`

`.als` 파일, `.mid` 파일을 직접 넣는 구조가 아니다.

## 6. 서버 실행 방법

서버는 별도 레포에서 실행한다.

경로:

- `/Users/simjuheun/Desktop/Team/mixing-extension`

실행 예시:

```bash
cd /Users/simjuheun/Desktop/Team/mixing-extension
./.venv/bin/python run.py
```

정상이라면 기본적으로 아래 주소에서 응답한다.

```text
http://127.0.0.1:5000
```

플러그인도 기본적으로 이 주소를 사용한다.

## 7. 플러그인 빌드 방법

### 가장 쉬운 방법: 터미널에서 Xcode 빌드

프로젝트 루트:

- `/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin`

빌드 명령:

```bash
xcodebuild -scheme 'VoltaAgentPlugin - Shared Code' \
  -project /Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Builds/MacOSX/VoltaAgentPlugin.xcodeproj \
  -configuration Debug build
```

### 빌드가 성공하면

정적 라이브러리/플러그인 산출물이 `Builds/MacOSX/build/Debug/` 쪽에 만들어진다.

실제 DAW에서 보이는 플러그인 파일 위치는 JUCE/Xcode 설정에 따라 다를 수 있으니, 처음에는 기존에 Ableton에서 읽고 있는 플러그인 바이너리가 어디인지 확인하는 것이 좋다.

## 8. Ableton에서 플러그인 열기

일반적인 흐름:

1. Ableton 실행
2. 플러그인 목록에서 `VoltaAgentPlugin` 찾기
3. 마스터 트랙 또는 전용 control 트랙에 삽입
4. 플러그인 UI 열기

현재 이 플러그인은 **오디오를 직접 처리하는 이펙터**보다는, **중앙 제어 패널**에 가깝다.

즉 추천 사용 위치는:

- 마스터 트랙
- 별도 control track

## 9. 처음 사용하는 사람이 따라야 할 실제 사용 순서

### 단계 1. stem WAV 폴더 준비

예를 들어 아래처럼 폴더를 하나 만든다.

```text
/Users/yourname/Desktop/MySong/stems/
  01_Kick.wav
  02_Bass.wav
  03_Guitar.wav
  04_Vocal.wav
```

중요:

- 여기에는 **분석하고 싶은 WAV만** 넣는 것이 좋다
- 레퍼런스 파일, 리턴 프린트, 마스터 bounce가 섞이면 같이 분석된다

### 단계 2. 플러그인에서 stem 폴더 선택

메인 화면에서:

- `Choose Stem Folder`

버튼을 눌러 WAV 폴더를 고른다.

### 단계 3. 분석 실행

- `Analyze WAV Stems`

버튼을 누른다.

이때 내부적으로는 다음이 일어난다.

1. `POST /projects`
2. `POST /projects/<id>/upload` 여러 번
3. `GET /projects/<id>/analysis`

정상이라면 UI 상태 문구가 업로드/분석 중으로 바뀐다.

### 단계 4. 분석 완료 확인

완료되면:

- Status 영역 문구 변경
- Assistant 응답 갱신
- Action Preview에 간단한 분석 결과 표시
- `Advanced Debug`의 project session 정보 갱신

### 단계 5. 채팅 사용

아래 입력창에 장르나 요청을 입력한다.

예:

```text
이 곡의 장르는 Rock이야. 트랙 길이를 분석해줘
```

전송 방법:

- `Enter`: 전송
- `Shift + Enter`: 줄바꿈
- `Send` 버튼 클릭: 전송

정상이라면 내부적으로:

- `POST /projects/<id>/chat`

가 호출되고, Assistant 영역에 응답이 보인다.

## 10. 현재 UI에서 각 영역의 의미

### Status

현재 서버 연결/분석/대기 상태를 보여준다.

예:

- 서버 오프라인
- stem 분석 완료
- 모델 응답 생성 중

### Stem Folder

현재 선택된 WAV 폴더를 보여준다.

보통 한 번 정하면 자주 바꾸지 않는다.

### Assistant

서버가 준 응답을 보여준다.  
현재는 사람에게 읽히는 자연어 중심이다.

### Your Request

마지막으로 사용자가 보낸 요청을 보여준다.

### 메시지 입력창

사용자가 AI에게 요청을 입력하는 곳이다.

### Action Preview

현재는 간단한 결과/도구 호출 정보/분석 요약이 보일 수 있다.  
향후에는 machine-readable action preview가 이 영역에 더 구조적으로 들어갈 예정이다.

### Advanced Debug

개발용 상세 정보 영역이다.

포함 정보:

- 서버 URL
- server/session 상태
- project session id
- stem folder
- analysis 상태
- tracks
- explanation/preview
- activity log

서비스화 단계에서는 축소되거나 제거될 수 있다.

## 11. 지금 바로 테스트하기 좋은 문장

### 장르만 지정

```text
Rock
```

### 트랙 길이 확인

```text
트랙 길이를 분석해줘
```

### 그룹 버스 제안

```text
이 세션 기준으로 그룹 버스를 어떻게 나누면 좋을지 정리해줘
```

### gain staging 제안

```text
각 트랙의 초기 gain staging 기준을 간단히 제안해줘
```

### 레퍼런스/FX 프린트 구분

```text
레퍼런스 후보와 FX 프린트를 구분해서 알려줘
```

## 12. 자주 헷갈리는 점

### Q1. `.als` 파일을 선택하면 되나요?

아니다.  
현재는 **Ableton 프로젝트 파일이 아니라 stem WAV 폴더**를 선택해야 한다.

### Q2. MIDI 파일도 넣을 수 있나요?

현재는 아니다.  
지금 분석 세션은 WAV 중심으로 설계되어 있다.

### Q3. 채팅 결과가 Ableton 트랙에 자동 반영되나요?

아직 아니다.  
현재는 **분석 + 채팅 응답**까지이고, 실제 Ableton apply와의 완전 연결은 다음 단계다.

### Q4. 왜 응답이 느릴 때가 있나요?

서버가 OpenAI 호출을 하고 있고, 도구 호출까지 같이 일어나면 수 초에서 수십 초가 걸릴 수 있다.  
그래서 UI에 `Working...`과 상태 문구가 보이도록 되어 있다.

## 13. 문제 해결

### 서버가 연결되지 않는 경우

확인 순서:

1. `mixing-extension` 서버가 실제로 켜져 있는지
2. `http://127.0.0.1:5000/health`가 열리는지
3. `Advanced Debug`의 Server URL이 맞는지

### 분석 버튼을 눌러도 아무 일도 없는 경우

확인 순서:

1. stem 폴더를 선택했는지
2. 폴더 안에 `.wav` 파일이 있는지
3. `Advanced Debug`의 activity log에 에러가 찍히는지

### 버튼/UI 변경이 안 보이는 경우

대개 새 빌드를 DAW가 아직 안 읽은 경우다.

해결:

1. 플러그인 인스턴스를 삭제
2. 다시 로드
3. 필요하면 Ableton 재실행

### Enter를 눌렀는데 반응이 없는 것처럼 보이는 경우

실제로는 서버 요청이 진행 중일 수 있다.

확인:

- `Status` 문구
- `Send` 버튼의 `Working...`
- `Advanced Debug` activity log

## 14. 현재 기능 요약

현재 이 플러그인으로 가능한 것:

- stem WAV 폴더 선택
- 서버 ProjectSession 생성
- stem 업로드
- 분석 결과 조회
- 프로젝트 채팅 사용
- 분석 결과를 기반으로 한 믹싱 가이드 받기

현재 아직 안 되는 것:

- 채팅 결과 자동 apply
- 버스 생성/라우팅/팬닝/게인 자동 반영
- machine-readable action preview/apply 연결

## 15. 다음에 코드를 볼 때 추천 순서

JUCE 경험이 거의 없다면 아래 순서로 보면 덜 헷갈린다.

1. [README.md](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/README.md)
2. [README_USAGE.md](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/README_USAGE.md)
3. [Source/UI/ControllerView.cpp](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/UI/ControllerView.cpp)
4. [Source/UI/DebugPanel.cpp](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/UI/DebugPanel.cpp)
5. [Source/PluginProcessor.cpp](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/PluginProcessor.cpp)
6. [Source/Communication/SessionControlClient.h](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/Source/Communication/SessionControlClient.h)

이 순서면:

- 화면이 어떻게 보이는지
- 어떤 상태가 유지되는지
- 어떤 API를 때리는지

를 차례대로 이해할 수 있다.

## 16. 한 줄 요약

이 플러그인은 현재  
**“stem WAV를 서버에 올리고, 분석 결과를 바탕으로 채팅형 믹싱 가이드를 받는 JUCE 클라이언트”**  
로 이해하면 가장 정확하다.
