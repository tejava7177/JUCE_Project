#include "DSP/BiquadDsp.h"

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
    eqlab::BiquadDsp unityFilter;
    unityFilter.setCoefficients (
        eqlab::BiquadDsp::makeBellCoefficients (sampleRate, frequency, 0.0f, q));

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

    eqlab::BiquadDsp bellFilter;
    const auto coefficients = eqlab::BiquadDsp::makeBellCoefficients (
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

    // Q가 1/sqrt(2)인 2차 Pass 필터는 cutoff에서 약 -3 dB가 된다.
    // Low-pass는 cutoff 아래를 통과시키고 위를 줄여야 한다.
    constexpr float passFilterQ = 0.70710678f;
    const auto lowPassCoefficients = eqlab::BiquadDsp::makeLowPassCoefficients (
        sampleRate, frequency, passFilterQ);
    const auto lowPassAt100Hz = eqlab::BiquadDsp::magnitudeDbAt (
        lowPassCoefficients, sampleRate, 100.0);
    const auto lowPassAtCutoff = eqlab::BiquadDsp::magnitudeDbAt (
        lowPassCoefficients, sampleRate, frequency);
    const auto lowPassAt10kHz = eqlab::BiquadDsp::magnitudeDbAt (
        lowPassCoefficients, sampleRate, 10000.0);

    std::cout << "Low-pass 1 kHz: 100 Hz=" << lowPassAt100Hz
              << " dB, 1 kHz=" << lowPassAtCutoff
              << " dB, 10 kHz=" << lowPassAt10kHz << " dB\n";

    if (lowPassAt100Hz < -0.2
        || std::abs (lowPassAtCutoff + 3.0103) > 0.05
        || lowPassAt10kHz > -35.0)
    {
        std::cerr << "FAIL: Low-pass가 저역 통과/고역 감쇠 조건을 만족하지 않습니다.\n";
        return 1;
    }

    // High-pass는 같은 cutoff와 Q를 사용하지만 통과 방향이 반대여야 한다.
    const auto highPassCoefficients = eqlab::BiquadDsp::makeHighPassCoefficients (
        sampleRate, frequency, passFilterQ);
    const auto highPassAt100Hz = eqlab::BiquadDsp::magnitudeDbAt (
        highPassCoefficients, sampleRate, 100.0);
    const auto highPassAtCutoff = eqlab::BiquadDsp::magnitudeDbAt (
        highPassCoefficients, sampleRate, frequency);
    const auto highPassAt10kHz = eqlab::BiquadDsp::magnitudeDbAt (
        highPassCoefficients, sampleRate, 10000.0);

    std::cout << "High-pass 1 kHz: 100 Hz=" << highPassAt100Hz
              << " dB, 1 kHz=" << highPassAtCutoff
              << " dB, 10 kHz=" << highPassAt10kHz << " dB\n";

    if (highPassAt100Hz > -35.0
        || std::abs (highPassAtCutoff + 3.0103) > 0.05
        || highPassAt10kHz < -0.2)
    {
        std::cerr << "FAIL: High-pass가 저역 감쇠/고역 통과 조건을 만족하지 않습니다.\n";
        return 1;
    }

    // Low Shelf +6 dB는 저역에서 +6 dB, 전환 중심에서 +3 dB,
    // 고역에서 0 dB에 가까워야 한다.
    constexpr float shelfSlope = 1.0f;
    const auto lowShelfCoefficients = eqlab::BiquadDsp::makeLowShelfCoefficients (
        sampleRate, frequency, requestedGainDb, shelfSlope);
    const auto lowShelfAt100Hz = eqlab::BiquadDsp::magnitudeDbAt (
        lowShelfCoefficients, sampleRate, 100.0);
    const auto lowShelfAtCentre = eqlab::BiquadDsp::magnitudeDbAt (
        lowShelfCoefficients, sampleRate, frequency);
    const auto lowShelfAt10kHz = eqlab::BiquadDsp::magnitudeDbAt (
        lowShelfCoefficients, sampleRate, 10000.0);

    std::cout << "Low-shelf 1 kHz, +6 dB: 100 Hz=" << lowShelfAt100Hz
              << " dB, 1 kHz=" << lowShelfAtCentre
              << " dB, 10 kHz=" << lowShelfAt10kHz << " dB\n";

    if (std::abs (lowShelfAt100Hz - requestedGainDb) > 0.1
        || std::abs (lowShelfAtCentre - requestedGainDb * 0.5) > 0.05
        || std::abs (lowShelfAt10kHz) > 0.1)
    {
        std::cerr << "FAIL: Low Shelf가 목표 Gain의 선반 모양을 만들지 못했습니다.\n";
        return 1;
    }

    // High Shelf는 같은 설정에서 저역과 고역의 역할이 반대가 된다.
    const auto highShelfCoefficients = eqlab::BiquadDsp::makeHighShelfCoefficients (
        sampleRate, frequency, requestedGainDb, shelfSlope);
    const auto highShelfAt100Hz = eqlab::BiquadDsp::magnitudeDbAt (
        highShelfCoefficients, sampleRate, 100.0);
    const auto highShelfAtCentre = eqlab::BiquadDsp::magnitudeDbAt (
        highShelfCoefficients, sampleRate, frequency);
    const auto highShelfAt10kHz = eqlab::BiquadDsp::magnitudeDbAt (
        highShelfCoefficients, sampleRate, 10000.0);

    std::cout << "High-shelf 1 kHz, +6 dB: 100 Hz=" << highShelfAt100Hz
              << " dB, 1 kHz=" << highShelfAtCentre
              << " dB, 10 kHz=" << highShelfAt10kHz << " dB\n";

    if (std::abs (highShelfAt100Hz) > 0.1
        || std::abs (highShelfAtCentre - requestedGainDb * 0.5) > 0.05
        || std::abs (highShelfAt10kHz - requestedGainDb) > 0.1)
    {
        std::cerr << "FAIL: High Shelf가 목표 Gain의 선반 모양을 만들지 못했습니다.\n";
        return 1;
    }

    // Notch는 지정한 중심 주파수를 거의 완전히 상쇄하고,
    // 충분히 멀리 떨어진 주파수는 0 dB에 가깝게 통과시켜야 한다.
    const auto notchCoefficients = eqlab::BiquadDsp::makeNotchCoefficients (
        sampleRate, frequency, q);
    const auto notchAt100Hz = eqlab::BiquadDsp::magnitudeDbAt (
        notchCoefficients, sampleRate, 100.0);
    const auto notchAtCentre = eqlab::BiquadDsp::magnitudeDbAt (
        notchCoefficients, sampleRate, frequency);
    const auto notchAt10kHz = eqlab::BiquadDsp::magnitudeDbAt (
        notchCoefficients, sampleRate, 10000.0);

    std::cout << "Notch 1 kHz, Q 1.0: 100 Hz=" << notchAt100Hz
              << " dB, 1 kHz=" << notchAtCentre
              << " dB, 10 kHz=" << notchAt10kHz << " dB\n";

    if (notchAt100Hz < -0.2
        || notchAtCentre > -70.0
        || notchAt10kHz < -0.2)
    {
        std::cerr << "FAIL: Notch가 중심 주파수를 충분히 상쇄하지 못했습니다.\n";
        return 1;
    }

    std::cout << "PASS: Bell / Pass / Shelf / Notch 테스트를 모두 통과했습니다.\n";
    return 0;
}
