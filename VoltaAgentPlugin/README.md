# VoltaAgentPlugin

`VoltaAgentPlugin`은 JUCE 기반 클라이언트로, DAW 내부 또는 독립 실행형 앱에서 동작하는 **AI Mixing Assistant UI**를 목표로 한다. 이 프로젝트의 역할은 단순 오디오 이펙터가 아니라, 사용자와 서버 사이를 연결하는 **채팅 + 분석 상태 + 액션 실행 패널**을 제공하는 것이다.

팀원이 바로 실행 흐름을 확인하려면 [README_USAGE.md](/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin/README_USAGE.md)를 먼저 보는 것이 좋다.

## 목표

이 프로젝트의 최종 목표는 다음 흐름을 구현하는 것이다.

1. 사용자는 JUCE 클라이언트에서 곡과 트랙을 업로드한다.
2. 서버는 별도의 음악 분석 세션을 열어 트랙 길이, RMS, peak, LUFS, spectrum, onset 등 기초 분석을 수행한다.
3. 동시에 LLM 채팅 세션은 사용자의 장르, 요청 의도, 다음 액션을 관리한다.
4. JUCE 클라이언트는 분석 진행 상태를 보여주고, 분석 결과를 채팅 UI 안에서 설명한다.
5. 사용자는 제안된 액션을 승인하거나 건너뛸 수 있고, 이후 gain, trim, EQ, grouping 같은 메카니컬 믹스 기능으로 확장한다.

## 제품 기본 플로우

```text
사용자
  ↓
JUCE 클라이언트
  ↓
우리 서버
  ├─ LLM 채팅 세션
  └─ 음악 분석 세션
        ↓
     분석 결과 저장
```

### 각 구성요소의 역할

#### JUCE 클라이언트

- DAW 내부 또는 독립 실행 앱에서 사용자 인터페이스 제공
- 오디오 파일 업로드
- 채팅 UI 표시
- 서버 응답 기반으로 적용 가능한 액션 실행

#### LLM 채팅 세션

- 사용자의 자연어 요청 이해
- 다음 단계 제안
- 분석 세션에 작업 요청
- 분석 결과를 사용자에게 설명

#### 음악 분석 세션

- 오디오 파일 수신
- 트랙별 길이, RMS, peak, LUFS, spectrum, onset 등 분석
- 메카니컬 믹스 기능이 재사용할 기초 데이터 생성

## 기본 사용자 시나리오

### 0단계. 기본 정보 수집

초기 목표는 곡의 최소 정보를 수집하면서, 백그라운드에서 분석 세션을 바로 시작하는 것이다.

예상 사용자 경험:

```text
AI:
안녕하세요. 먼저 곡의 장르를 알려주세요.
예: Hip-hop, K-pop, Rock, R&B, EDM 등
```

동시에 내부에서는 다음이 일어난다.

1. JUCE가 프로젝트 또는 선택된 트랙 목록 확인
2. 서버에 analysis session 생성 요청
3. JUCE가 각 트랙의 원본 파일 또는 압축 파일 업로드
4. 서버가 트랙 분석 시작
5. LLM 채팅 세션은 사용자에게 장르 질문

중요한 점:

- 장르 질문과 트랙 업로드는 동시에 진행한다.
- 사용자가 장르를 답하는 동안 서버는 이미 길이, 포맷, 샘플레이트 등의 기초 분석을 진행한다.
- 이후 메카니컬 믹스 단계의 응답 시간을 줄이기 위한 구조다.

### 1단계. 메카니컬 믹스 시작

트랙 분석이 끝나면 AI는 사용자가 선택할 수 있는 액션을 제안한다.

예시:

```text
AI:
트랙 분석이 완료되었습니다.
이제 메카니컬 믹스를 시작할 수 있습니다.

먼저 다음 중 하나를 진행할 수 있습니다.

1. 트랙 정리 및 그룹핑
2. 트랙 길이 확인
3. 불필요한 무음 trim
4. 기본 gain balance
5. low cut / high cut EQ
6. 킥-베이스 충돌 확인
```

사용자는 직접 요청하거나, AI 제안에 응답할 수 있다.

예시:

```text
사용자:
각 트랙 길이부터 알려줘.
```

또는:

```text
AI:
먼저 트랙별 길이를 확인하면 세션 구조를 파악하기 좋습니다.
각 트랙의 길이를 확인하시겠습니까?

사용자:
응.
```

결과 출력 예시:

```text
AI:
현재 업로드된 트랙 길이는 다음과 같습니다.

Kick.wav — 3:12
Snare.wav — 3:12
Lead Vocal Verse.wav — 0:42
Lead Vocal Chorus.wav — 0:38
Bass.wav — 3:12
```

## 테스트용 최소 플로우

초기 검증 목표는 다음 세 가지다.

1. JUCE ↔ 서버 ↔ LLM 세션 연결 확인
2. JUCE ↔ 서버 ↔ 음악 분석 세션이 분리되어 동작하는지 확인
3. 채팅 명령으로 음악 분석 결과를 다시 호출할 수 있는지 확인

테스트 시나리오:

```text
[사용자 플러그인/앱 실행]

AI:
프로젝트 분석을 시작하겠습니다.
먼저 장르를 알려주세요.

[동시에 JUCE → 서버로 전체 트랙 파일 업로드]

사용자:
K-pop이야.

AI:
좋습니다. K-pop 기준으로 메카니컬 믹스를 준비하겠습니다.
각 트랙의 길이를 알고 싶으십니까?

사용자:
예.

[LLM 서버 → 음악분석 세션에 track length 요청]
[음악분석 세션 → track length 결과 반환]
[LLM 세션 → 사용자에게 자연어로 응답]

AI:
업로드된 트랙의 길이는 다음과 같습니다.

1. Kick — 3분 12초
2. Snare — 3분 12초
3. Bass — 3분 12초
4. Lead Vocal Verse — 43초
5. Lead Vocal Chorus — 38초

다음으로 트랙 그룹핑을 진행할 수 있습니다.
```

사용자가 거절하면:

```text
AI:
알겠습니다. 트랙 길이 확인은 건너뛰겠습니다.
다음 단계로 트랙 그룹핑이나 기본 gain balance를 진행할 수 있습니다.
```

## UI 설계 방향

이 프로젝트의 UI는 단순 채팅창이 아니라 **채팅 + 액션 제안 + 분석 상태 + 적용 버튼**을 함께 가지는 구조를 지향한다.

예시:

```text
┌─────────────────────────────┐
│ AI Mixing Assistant         │
├─────────────────────────────┤
│ AI: 장르를 알려주세요.      │
│ User: K-pop                 │
│ AI: 트랙 길이를 확인할까요? │
│                             │
│ [예] [아니오]               │
├─────────────────────────────┤
│ Analysis Status             │
│ Uploading tracks... 80%     │
│ Analyzing tracks...         │
└─────────────────────────────┘
```

채팅 메시지 타입 예시:

```ts
type ChatMessage =
  | UserMessage
  | AssistantMessage
  | ActionSuggestion
  | AnalysisResultMessage
  | ErrorMessage;
```

## 서버 세션 구조

이 프로젝트는 채팅 세션과 분석 세션을 분리한 상위 구조를 전제로 한다.

```text
ProjectSession
  ├─ ChatSession
  └─ AnalysisSession
```

### ProjectSession

곡 하나 또는 DAW 프로젝트 하나를 대표하는 상위 세션.

```json
{
  "project_session_id": "proj_123",
  "user_id": "user_001",
  "genre": "K-pop",
  "created_at": "2026-05-03T16:00:00+09:00"
}
```

### ChatSession

LLM 대화 전용 세션.

```json
{
  "chat_session_id": "chat_123",
  "project_session_id": "proj_123",
  "messages": [],
  "current_step": "collect_genre"
}
```

### AnalysisSession

음악 분석 전용 세션. 오디오 파일, waveform, feature, 분석 결과를 분리 저장하고 재사용한다.

이 분리가 필요한 이유:

## Machine-Readable Action Draft

현재 `VoltaAgentPlugin`은 두 개의 서로 다른 서버 축을 함께 사용하고 있다.

- `POST /projects/*`
  - stem WAV 업로드
  - AnalysisSession 기반 분석
  - ProjectSession 채팅
- `/scan`, `/session-summary`, `/plan-actions`, `/apply-actions`
  - Ableton 세션 스캔
  - 디바이스/파라미터 기반 apply

지금까지는 `/projects/<id>/chat` 응답이 주로 자연어 설명 중심이었다.  
하지만 최종 목표는 채팅 결과를 **machine-readable action** 으로 바꾸고, 이를 Ableton apply 축과 연결하는 것이다.

### 목표

```text
사용자 채팅 요청
  ↓
ProjectSession chat
  ↓
assistant_message + actions[]
  ↓
JUCE plugin preview
  ↓
Ableton apply 축으로 전달
  ↓
M4L / executor가 실제 반영
```

즉 서버 응답은 앞으로 아래 두 층을 동시에 제공해야 한다.

1. 사람이 읽는 설명
2. 플러그인이 실행 가능한 구조화된 action 목록

### 응답 초안

예시:

```json
{
  "assistant": "기타를 좌우로 나누고 FX 프린트를 별도 버스로 정리하겠습니다.",
  "current_step": "mechanical_mix",
  "genre": "Rock",
  "actions": [
    {
      "type": "route_track_to_bus",
      "track_name": "123 3-Guitar_Intro.wav",
      "target_bus": "Guitars"
    },
    {
      "type": "route_track_to_bus",
      "track_name": "123 2-Guitar_Solo.wav",
      "target_bus": "Guitars"
    },
    {
      "type": "set_pan",
      "track_name": "123 3-Guitar_Intro.wav",
      "value": -35
    },
    {
      "type": "set_pan",
      "track_name": "123 2-Guitar_Solo.wav",
      "value": 35
    },
    {
      "type": "set_gain_target",
      "track_name": "123 A-Reverb.wav",
      "target_peak_dbfs": -19
    }
  ]
}
```

### 1차 액션 타입 초안

초기 MVP에서는 아래 액션만 먼저 정의하는 것이 적절하다.

#### Session Actions

- `create_bus`
- `route_track_to_bus`
- `mute_track`
- `mark_as_reference`
- `set_pan`
- `set_gain_target`
- `set_hpf`

#### Parameter Actions

- `set_parameter_value`
- `set_eq_gain`
- `set_eq_hpf`
- `set_compressor_threshold`
- `set_utility_gain`

이 구분이 필요한 이유는 다음과 같다.

- `Session Actions`
  - 세션 구조, 라우팅, 레퍼런스 관리, 팬/게인 같은 정리 단계
- `Parameter Actions`
  - 기존 `/apply-actions` 축과 더 직접적으로 연결되는 장치/파라미터 조정

### 액션 스키마 초안

공통 필드:

```json
{
  "id": "action_001",
  "type": "set_pan",
  "scope": "session",
  "track_name": "123 3-Guitar_Intro.wav",
  "confidence": 0.92,
  "reason": "인트로 기타를 좌측에 배치해 스테레오 분리를 만듭니다."
}
```

타입별 필드 예시:

#### `create_bus`

```json
{
  "type": "create_bus",
  "bus_name": "Guitars"
}
```

#### `route_track_to_bus`

```json
{
  "type": "route_track_to_bus",
  "track_name": "123 2-Guitar_Solo.wav",
  "target_bus": "Guitars"
}
```

#### `set_pan`

```json
{
  "type": "set_pan",
  "track_name": "123 2-Guitar_Solo.wav",
  "value": 35,
  "unit": "percent"
}
```

#### `set_gain_target`

```json
{
  "type": "set_gain_target",
  "track_name": "123 1-Dr_PercLoop.wav",
  "target_peak_dbfs": -9.0
}
```

#### `set_hpf`

```json
{
  "type": "set_hpf",
  "track_name": "123 A-Reverb.wav",
  "frequency_hz": 180.0,
  "slope_db_per_oct": 12
}
```

### JUCE 플러그인에서의 역할

`VoltaAgentPlugin`은 이 액션들을 바로 실행하지 않고, 우선 아래 순서로 다룬다.

1. `assistant` 메시지를 채팅 UI에 표시
2. `actions[]`를 preview 카드에 구조적으로 표시
3. 사용자 승인 시 apply 축으로 전달
4. apply 결과를 activity log 와 채팅에 다시 반영

즉 플러그인의 역할은 단순 렌더링이 아니라:

- 채팅 결과 해석
- action preview
- 승인/거절
- apply orchestration

까지 포함한다.

### Ableton apply 축과의 연결 초안

초기에는 아래 두 방식 중 하나를 선택할 수 있다.

#### 옵션 A. 기존 `/apply-actions` 확장

- 장점:
  - 기존 apply 경로 재사용 가능
- 단점:
  - 현재 `/apply-actions`가 파라미터 중심이라 세션 구조 액션과 맞지 않을 수 있음

#### 옵션 B. 별도 apply endpoint 추가

예:

```text
POST /projects/<id>/apply
```

- 장점:
  - `ProjectSession` 기반 structured action 과 의미적으로 더 잘 맞음
- 단점:
  - 새 executor 경로 정의 필요

현 시점 초안으로는 **옵션 B**가 더 자연스럽다.

### 구현 단계 제안

#### 단계 1

- `/projects/<id>/chat` 응답에 `actions[]` 추가
- JUCE plugin 은 preview 만 표시

#### 단계 2

- `actions[]` 승인 UI 추가
- action 타입별 표시 형식 정리

#### 단계 3

- `actions[]`를 Ableton apply 축으로 전달
- M4L / executor 가 실제 트랙/버스/파라미터 반영

#### 단계 4

- apply 결과를 다시 채팅 세션에 반영
- 예:
  - 성공
  - 일부 실패
  - 사용자 확인 필요

### 현재 한계

- 현재 `/projects/<id>/chat`은 자연어 응답 중심
- 현재 `/apply-actions`는 세션 구조 변경보다 파라미터 변경에 더 적합
- stem 분석 기준의 track name 과 Ableton scan 기준 track name 의 매핑 정책이 아직 필요
- bus 생성, 라우팅, reference 관리 같은 액션은 아직 executor 스펙이 없음

### 한 줄 요약

다음 단계의 핵심은  
`ProjectSession chat 결과를 assistant_message + actions[] 구조로 확장하고, 이 actions[]를 Ableton apply 축과 연결하는 것`이다.

- LLM 채팅은 대화 맥락 관리에 집중
- 음악 분석은 파일, waveform, feature, 분석 결과 관리에 집중
- 이후 trim, EQ, compressor, panning, grouping 등 12개 기능이 같은 분석 데이터를 재사용 가능

## 이 프로젝트에서 우선 구현할 것

`VoltaAgentPlugin`에서는 우선 아래를 클라이언트 관점에서 구현한다.

1. 서버 연결 상태 표시
2. 프로젝트/트랙 업로드 상태 표시
3. 채팅 메시지 UI
4. 액션 제안 UI
5. 분석 결과 표시 UI
6. 승인/거절/다음 단계 버튼

즉, 이 프로젝트의 핵심은 오디오 DSP보다 **세션 오케스트레이션과 사용자 인터페이스**다.

## 구현 메모

- JUCE 클라이언트는 장르 질문과 파일 업로드를 동시에 처리할 수 있어야 한다.
- 장시간 작업(업로드, 분석 요청, LLM 요청)은 오디오 스레드와 분리해야 한다.
- 분석 상태, 업로드 진행률, 채팅 단계는 모두 UI에서 명시적으로 보여야 한다.
- 액션 제안은 단순 텍스트가 아니라, 사용자가 바로 승인/거절할 수 있는 형태가 바람직하다.
- 이후 12개 메카니컬 믹스 기능을 붙일 수 있도록 메시지 타입과 세션 구조를 미리 분리해두어야 한다.

## 현재 문서의 목적

이 README는 `VoltaAgentPlugin`을 어떤 제품으로 만들 것인지 합의하기 위한 **상위 설계 문서**다. 이후 실제 구현은 아래 순서로 세분화한다.

1. 채팅/상태 UI 구조 정의
2. 업로드 및 analysis session 생성 API 연결
3. chat session 연결
4. track length 조회 같은 최소 분석 기능 연결
5. 액션 제안과 승인 플로우 연결
6. 이후 12개 메카니컬 믹스 기능 확장
