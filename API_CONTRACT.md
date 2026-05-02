# API_CONTRACT

## Status

Draft. This document is an initial API contract proposal for communication between the JUCE plugin and the external mixing analysis server. It is not final and may evolve, but any field changes should be recorded here before or alongside implementation changes.

## Purpose

The plugin sends a mixing analysis request to the server. The server returns structured recommendation data such as EQ, gain, and compressor-related suggestions.

This draft is aligned to the current parameter IDs used by `VoltaAgentPlugin`. Server responses should use plugin-recognized parameter IDs directly where possible.

## Ownership

- Plugin client: `/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin`
- Server implementation: `/Users/simjuheun/Desktop/Team/mixing-extension`

## Endpoint

- Method: `POST`
- Path: `/api/mix/analyze`
- Content-Type: `application/json`

## Request Schema

Top-level fields:

- `trackType`: string
- `sampleRate`: number
- `bpm`: number
- `features`: object
- `currentParams`: object

### Request Example

```json
{
  "trackType": "bass",
  "sampleRate": 44100,
  "bpm": 100,
  "features": {
    "rms": -18.2,
    "peak": -6.1,
    "spectralCentroid": 1200
  },
  "currentParams": {
    "gain_db": 0.0,
    "low_cut_freq_hz": 80.0,
    "presence_db": 0.0,
    "compression_amount": 0.25,
    "warmth_amount": 0.1
  }
}
```

### Request Field Notes

- `trackType`: logical source category such as `bass`, `vocal`, `drums`, or `guitar`
- `sampleRate`: current analysis or session sample rate. In the plugin, this is expected to be captured during `prepareToPlay()` and stored for inclusion in the request.
- `bpm`: tempo context if available
- `features`: Draft. Feature extraction is not finalized in the plugin yet, so this object should remain provisional until real extraction code exists.
- `features.rms`: Draft RMS estimate in dB
- `features.peak`: Draft peak estimate in dB
- `features.spectralCentroid`: Draft spectral centroid estimate in Hz
- `currentParams`: current plugin parameter snapshot sent for recommendation context
- `currentParams.gain_db`: current plugin gain parameter in dB
- `currentParams.low_cut_freq_hz`: current low-cut frequency parameter in Hz
- `currentParams.presence_db`: current presence parameter in dB
- `currentParams.compression_amount`: normalized compression amount currently used by the plugin
- `currentParams.warmth_amount`: normalized warmth amount currently used by the plugin

## Response Schema

Top-level fields:

- `summary`: string
- `suggestions`: array

Each suggestion object:

- `parameter`: string
- `action`: string
- `value`: number
- `unit`: string
- `reason`: string

### Response Example

```json
{
  "summary": "Bass is slightly muddy.",
  "suggestions": [
    {
      "parameter": "presence_db",
      "action": "cut",
      "value": -2.5,
      "unit": "dB",
      "reason": "Reduce excessive upper-mid emphasis."
    },
    {
      "parameter": "compression_amount",
      "action": "increase",
      "value": 0.4,
      "unit": "normalized",
      "reason": "Add more control to bass dynamics."
    }
  ]
}
```

### Response Field Notes

- `summary`: short natural-language analysis summary for display
- `suggestions`: machine-readable recommendation list
- `parameter`: target plugin parameter ID. For the current draft, valid values are `gain_db`, `low_cut_freq_hz`, `presence_db`, `compression_amount`, and `warmth_amount`.
- `action`: intended operation such as `boost`, `cut`, `increase`, or `decrease`
- `value`: suggested amount
- `unit`: unit such as `dB`, `Hz`, `ratio`, or similar
- `reason`: concise explanation for the recommendation

## Draft Compatibility Guidance

- The plugin should treat unknown response fields as ignorable unless explicitly required later.
- The plugin should ignore any `suggestions[].parameter` value that does not match a known plugin parameter ID.
- The server should avoid renaming existing fields without updating this document first.
- The server should emit plugin-recognized parameter IDs directly for suggestion targets whenever possible.
- If plugin parameter names and API parameter names diverge in the future, the mapping must be documented explicitly.
- Real-time audio processing must not depend on synchronous network completion.

## Open Draft Questions

- Should compressor recommendations use separate fields such as threshold, ratio, attack, and release?
- Should the request include stereo image, LUFS, crest factor, or band energy metrics?
- Should the response include confidence scores or priority ordering?
- Should the API support multiple simultaneous suggestions for the same parameter group?
