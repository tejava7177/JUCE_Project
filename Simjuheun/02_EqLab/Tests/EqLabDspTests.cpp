#include "DSP/BiquadBellDsp.h"

#include <cmath>
#include <iostream>

namespace
{
bool nearlyEqual (float actual, float expected, float tolerance = 0.0001f)
{
    return std::abs (actual - expected) <= tolerance;
}
}

int main()
{
    constexpr double sampleRate = 48000.0;
    constexpr float frequency = 1000.0f;
    constexpr float q = 1.0f;

    // Gain 0 dB에서는 Bell EQ가 입력을 전혀 바꾸지 않아야 한다.
    eqlab::BiquadBellDsp unityFilter;
    unityFilter.setCoefficients (
        eqlab::BiquadBellDsp::makeBellCoefficients (sampleRate, frequency, 0.0f, q));

    const float input[] { 1.0f, 0.25f, -0.5f, 0.75f, 0.0f };
    for (const auto inputSample : input)
    {
        const auto outputSample = unityFilter.processSample (inputSample);
        if (! nearlyEqual (outputSample, inputSample))
        {
            std::cerr << "FAIL: Gain 0 dB에서 입력과 출력이 다릅니다.\n";
            return 1;
        }
    }

    // 1 kHz에 +6 dB Bell을 설정하고 충분히 긴 1 kHz sine을 통과시킨다.
    // 필터가 안정된 뒤의 RMS 비율이 실제로 약 +6 dB인지 확인한다.
    constexpr float requestedGainDb = 6.0f;
    constexpr int totalSamples = 48000;
    constexpr int measurementStart = totalSamples / 2;
    constexpr double pi = 3.14159265358979323846;

    eqlab::BiquadBellDsp bellFilter;
    const auto coefficients = eqlab::BiquadBellDsp::makeBellCoefficients (
        sampleRate, frequency, requestedGainDb, q);
    bellFilter.setCoefficients (coefficients);

    double inputPower = 0.0;
    double outputPower = 0.0;

    for (int sample = 0; sample < totalSamples; ++sample)
    {
        const auto inputSample = static_cast<float> (
            0.25 * std::sin (2.0 * pi * frequency * sample / sampleRate));
        const auto outputSample = bellFilter.processSample (inputSample);

        if (sample >= measurementStart)
        {
            inputPower += static_cast<double> (inputSample) * inputSample;
            outputPower += static_cast<double> (outputSample) * outputSample;
        }
    }

    const auto measuredGainDb = 10.0 * std::log10 (outputPower / inputPower);

    std::cout << "Bell 설정: 1 kHz, +6 dB, Q 1.0\n";
    std::cout << "계수: b0=" << coefficients.b0
              << ", b1=" << coefficients.b1
              << ", b2=" << coefficients.b2
              << ", a1=" << coefficients.a1
              << ", a2=" << coefficients.a2 << "\n";
    std::cout << "1 kHz 실제 측정 Gain: " << measuredGainDb << " dB\n";

    if (std::abs (measuredGainDb - requestedGainDb) > 0.05)
    {
        std::cerr << "FAIL: 중심 주파수의 측정 Gain이 +6 dB가 아닙니다.\n";
        return 1;
    }

    std::cout << "PASS: Biquad Bell EQ 테스트를 모두 통과했습니다.\n";
    return 0;
}
