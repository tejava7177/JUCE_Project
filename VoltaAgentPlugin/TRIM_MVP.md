# Trim MVP Spec

`VoltaAgentPlugin`의 trim 기능은 단순한 자연어 응답이 아니라,

1. 서버가 무음 구간을 분석하고
2. 채팅 결과를 machine-readable action 으로 반환하고
3. JUCE 플러그인이 preview / 승인 UI 를 제공하고
4. Ableton apply 축(M4L executor)이 실제 clip trim 을 반영하는

구조로 구현한다.

이 문서는 그 1차 MVP 범위를 고정하기 위한 설계 문서다.

## 목표 사용자 요청

예시:

```text
현재 프로젝트의 각 트랙에서 무음인 부분을 없애줘.
```

사용자가 기대하는 결과:

- 각 트랙의 불필요한 무음이 줄어든다
- 수작업으로 clip start/end 를 당기지 않아도 된다
- 적용 전 어떤 트랙이 얼마나 trim 되는지 확인할 수 있다

## 1차 MVP 범위

이번 단계에서는 아래만 지원한다.

- leading silence 제거
- trailing silence 제거
- 실제 오디오 파일을 파괴적으로 수정하지 않음
- Ableton clip 의 시작점 / 끝점만 조정
- action preview 후 사용자 승인 방식

이번 단계에서 하지 않는 것:

- 중간 무음 구간을 잘라 여러 clip 으로 분할
- crossfade 자동 생성
- MIDI clip trim
- mp3 등 비권장 포맷 대상 trim
- 다른 DAW executor 구현

한 줄로 정리하면:

`앞뒤 무음만 안전하게 잘라주는 비파괴적 clip trim`

## 왜 이렇게 시작하나

중간 무음을 잘라내는 기능은 사용자 체감은 크지만 리스크도 크다.

- clip 분할이 필요하다
- section 경계가 틀리면 연주 흐름이 깨질 수 있다
- 반환 action schema 도 더 복잡해진다

반면 앞뒤 무음 trim 은:

- 구현이 단순하다
- 사용자가 의도 이해하기 쉽다
- preview / rollback 설계가 쉽다

따라서 1차 MVP는 leading / trailing silence trim 으로 제한한다.

## 시스템 역할 분리

### 서버

- WAV stem 분석
- 트랙별 leading / trailing silence 계산
- trim 제안 생성
- 채팅 응답에 action 포함

### JUCE 플러그인

- 사용자 자연어 요청 전달
- assistant 설명 표시
- trim action preview 표시
- 승인 / 취소 UI 제공

### Ableton M4L executor

- 승인된 trim action 수신
- 실제 Ableton clip start / end 조정
- 성공 / 실패 ack 반환

## 분석 기준

서버는 트랙별로 최소 아래 필드를 계산해야 한다.

```json
{
  "track_name": "456 4-Bass_Main.wav",
  "duration_seconds": 171.034,
  "leading_silence_sec": 0.118,
  "trailing_silence_sec": 0.452,
  "trim_start_sec": 0.118,
  "trim_end_sec": 170.582,
  "silence_threshold_dbfs": -55.0,
  "min_silence_duration_sec": 0.08
}
```

### 권장 기준값

- `silence_threshold_dbfs`: `-55 dBFS`
- `min_silence_duration_sec`: `0.08 ~ 0.15`

초기 기본값 제안:

- sustained source: `-55 dBFS`, `0.12 sec`
- drum/percussive source: `-50 dBFS`, `0.08 sec`

하지만 1차 MVP 에서는 장르/소스별 adaptive threshold 까지는 하지 않고,
우선 고정 기준으로 시작해도 충분하다.

## Action Schema 초안

### 단일 트랙 trim action

```json
{
  "id": "action_trim_001",
  "type": "trim_clip_edges",
  "scope": "session",
  "track_name": "456 4-Bass_Main.wav",
  "clip_name": "456 4-Bass_Main.wav",
  "leading_silence_sec": 0.118,
  "trailing_silence_sec": 0.452,
  "trim_start_sec": 0.118,
  "trim_end_sec": 170.582,
  "keep_duration_sec": 170.464,
  "mode": "non_destructive_clip_trim",
  "confidence": 0.93,
  "reason": "클립 앞뒤의 무음 구간을 제거해 세션 정리와 편집성을 높입니다."
}
```

### 여러 트랙 응답 예시

```json
{
  "assistant": "앞뒤 무음이 있는 트랙을 정리할 수 있습니다. 적용 전 preview 를 확인해주세요.",
  "current_step": "mechanical_mix",
  "actions": [
    {
      "id": "action_trim_bass_main",
      "type": "trim_clip_edges",
      "track_name": "456 4-Bass_Main.wav",
      "clip_name": "456 4-Bass_Main.wav",
      "leading_silence_sec": 0.118,
      "trailing_silence_sec": 0.452,
      "trim_start_sec": 0.118,
      "trim_end_sec": 170.582,
      "keep_duration_sec": 170.464,
      "mode": "non_destructive_clip_trim",
      "confidence": 0.93
    },
    {
      "id": "action_trim_guitar_intro",
      "type": "trim_clip_edges",
      "track_name": "456 31-Guitar_Intro.wav",
      "clip_name": "456 31-Guitar_Intro.wav",
      "leading_silence_sec": 8.217,
      "trailing_silence_sec": 0.000,
      "trim_start_sec": 8.217,
      "trim_end_sec": 171.034,
      "keep_duration_sec": 162.817,
      "mode": "non_destructive_clip_trim",
      "confidence": 0.88
    }
  ]
}
```

## 실행 정책

### trim 대상

trim action 은 아래 조건을 만족할 때만 생성한다.

- `leading_silence_sec > min_trim_threshold_sec`
  또는
- `trailing_silence_sec > min_trim_threshold_sec`

권장 기본값:

- `min_trim_threshold_sec = 0.05`

즉 10ms, 20ms 같은 매우 작은 경계 흔들림은 action 으로 만들지 않는다.

### trim 제외 조건

아래 경우는 action 을 만들지 않는다.

- 전체가 거의 무음인 파일
- trim 후 길이가 너무 짧아지는 파일
- reference / print mix 로 분류된 파일
- 사용자가 보호 대상으로 지정한 트랙

예시:

```json
{
  "type": "skip_trim",
  "track_name": "456.wav",
  "reason": "reference_mix"
}
```

## JUCE UI 요구사항

trim 요청이 들어오면 플러그인은 아래를 보여줘야 한다.

### 채팅 응답

- 왜 trim 을 제안하는지
- 어떤 트랙이 대상인지

### action preview

예:

```text
Bass_Main: 앞 0.12s 제거, 뒤 0.45s 제거
Guitar_Intro: 앞 8.22s 제거
Vocal_Main_Dry: trim 없음
```

### 사용자 승인

- `Apply Trim`
- `Skip`

가능하면 이후 단계에서:

- 트랙별 체크박스
- 전체 적용 / 일부 적용

도 추가할 수 있다.

## Ableton executor semantics

M4L executor 는 `trim_clip_edges` action 을 받으면 아래처럼 동작한다.

1. `track_name` 기준으로 대상 track 찾기
2. `clip_name` 또는 현재 clip reference 찾기
3. clip start marker / end marker 조정
4. 비파괴적으로 clip boundary 변경
5. 결과 ack 반환

### 반환 예시

```json
{
  "status": "applied",
  "action_id": "action_trim_bass_main",
  "track_name": "456 4-Bass_Main.wav",
  "applied_trim_start_sec": 0.118,
  "applied_trim_end_sec": 170.582
}
```

실패 예시:

```json
{
  "status": "error",
  "action_id": "action_trim_bass_main",
  "track_name": "456 4-Bass_Main.wav",
  "error": "clip_not_found"
}
```

## 위험 요소

### 1. stem 기준 이름과 Live track 기준 이름 불일치

현재 stem WAV 이름과 Ableton track name 이 정확히 일치하지 않을 수 있다.

예:

- stem: `456 4-Bass_Main.wav`
- Live track: `Bass_Main`

따라서 장기적으로는 아래 중 하나가 필요하다.

- 업로드 시 track mapping table 저장
- stem filename -> Ableton track name normalization

### 2. clip 이 여러 개 있는 경우

한 track 안에 clip 이 여러 개면 “어느 clip 을 trim 할 것인가”가 애매하다.

1차 MVP 는 아래 전제를 둔다.

- 각 stem track 은 하나의 대표 audio clip 을 가진다

여러 clip 편집은 2차 범위로 넘긴다.

### 3. reference / FX print 처리

`456.wav`, `A-Reverb`, `B-Delay` 같은 트랙은 trim 하더라도 실익이 적거나 오히려 불편할 수 있다.

따라서 기본 정책은:

- reference mix: trim 제외
- FX print: 기본 제외, 필요 시 옵션으로 허용

## 구현 단계

### 단계 1. 서버 분석 필드 추가

- `leading_silence_sec`
- `trailing_silence_sec`
- `trim_start_sec`
- `trim_end_sec`

### 단계 2. `/projects/<id>/chat` 응답에 trim action 추가

- 자연어 응답 + `actions[]`

### 단계 3. JUCE preview UI

- trim 대상 트랙 목록
- 제거 시간 표시
- 승인 버튼

### 단계 4. Ableton apply 축 연결

- `trim_clip_edges` executor 구현
- ack / error 처리

## 1차 완료 기준

아래가 되면 trim MVP 는 1차 완료다.

1. 사용자가 `무음 부분을 없애줘` 라고 요청할 수 있다
2. 서버가 leading / trailing silence 를 분석한다
3. 채팅 응답이 trim actions 를 반환한다
4. 플러그인이 trim preview 를 보여준다
5. 승인 시 Ableton clip trim 이 실제 반영된다

## 한 줄 요약

trim MVP 의 핵심은

`서버가 앞뒤 무음을 분석하고, 이를 trim_clip_edges action 으로 반환하고, JUCE 승인 후 Ableton clip boundary 를 비파괴적으로 조정하는 것`

이다.
