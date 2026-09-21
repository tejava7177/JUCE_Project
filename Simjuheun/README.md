# Simjuheun DSP Lab

직접 만든 작은 플러그인을 순서대로 쌓아가는 학습 공간입니다.

| 순서 | 프로젝트 | 배우는 내용 |
|---|---|---|
| 01 | `01_GainLab/` | Gain, smoothing, Dry/Wet, Delay |
| 02 | `02_EqLab/` | IIR, Biquad, Bell, Pass, Shelf, Notch |
| 03 | `03_DynamicEqLab/` | Band detector, Threshold, Ratio, Attack/Release, Dynamic Bell EQ |
| 04 | `04_CompressorLab/` | Envelope Follower, Gain Reduction, Ratio, Makeup Gain, Stereo Link |
| 05 | `05_DeEsserLab/` | Wide-band De-esser, Sibilance Detector, Detector Listen, Gain Reduction |

## 학습 기록

- [처음 만든 오디오 플러그인: Gain에서 Biquad EQ까지](GAIN_TO_EQ_DSP_STUDY.md)

각 프로젝트는 독립적으로 빌드할 수 있으며, DSP 코드는 `Source/DSP`에 분리해
DAW 없이도 작은 샘플 배열로 테스트합니다.
