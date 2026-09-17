#pragma once

#include <cmath>

namespace gainlab
{

// Gain 계산만 담당하는 작은 DSP 클래스다.
// JUCE나 DAW를 전혀 모르기 때문에 플러그인 밖에서도 같은 코드를 테스트할 수 있다.
class GainDsp
{
public:
    static float decibelsToLinear (float gainDb) noexcept
    {
        // dB는 사람이 조절하기 편한 단위이고, 샘플에는 선형 배수를 곱해야 한다.
        // 예: 0 dB -> 1.0배, 약 -6.0206 dB -> 0.5배
        return std::pow (10.0f, gainDb / 20.0f);
    }

    static float processSample (float inputSample, float linearGain) noexcept
    {
        // 실제 Gain DSP의 핵심이다. 입력 샘플 하나에 현재 Gain을 곱한다.
        return inputSample * linearGain;
    }

    static void applyGain (float* samples, int numSamples, float linearGain) noexcept
    {
        // samples는 한 채널의 첫 번째 샘플을 가리킨다.
        // 배열의 모든 샘플을 방문하면서 같은 Gain을 제자리에서 곱한다.
        for (int sample = 0; sample < numSamples; ++sample)
            samples[sample] = processSample (samples[sample], linearGain);
    }
};

} // namespace gainlab
