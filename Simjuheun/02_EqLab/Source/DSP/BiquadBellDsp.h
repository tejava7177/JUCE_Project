#pragma once

#include <algorithm>
#include <cmath>

namespace eqlab
{

// Biquad 한 개가 사용하는 다섯 개의 정규화된 계수다.
// b 계수는 현재·과거 입력에, a 계수는 과거 출력에 곱한다.
struct BiquadCoefficients
{
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
};

// Frequency, Gain, Q를 Bell EQ 공식에 넣어 계수를 만들고 샘플을 처리한다.
// JUCE나 DAW에 의존하지 않으므로 작은 샘플 배열만으로도 테스트할 수 있다.
class BiquadBellDsp
{
public:
    static BiquadCoefficients makeBellCoefficients (double sampleRate,
                                                     float frequencyHz,
                                                     float gainDb,
                                                     float q) noexcept
    {
        constexpr double pi = 3.14159265358979323846;

        const auto safeSampleRate = std::max (1.0, sampleRate);
        const auto maximumFrequency = static_cast<float> (safeSampleRate * 0.499);
        const auto safeFrequency = std::clamp (frequencyHz, 1.0f, maximumFrequency);
        const auto safeQ = std::max (0.01f, q);

        // A는 사용자가 입력한 dB Gain을 Bell 공식에서 사용하는 선형 값으로 바꾼 것이다.
        const auto a = std::pow (10.0, static_cast<double> (gainDb) / 40.0);

        // 같은 1 kHz라도 Sample Rate에 따라 한 주기를 구성하는 샘플 수가 달라지므로
        // Frequency를 Sample Rate에 대한 각도(omega)로 변환한다.
        const auto omega = 2.0 * pi * static_cast<double> (safeFrequency) / safeSampleRate;
        const auto alpha = std::sin (omega) / (2.0 * static_cast<double> (safeQ));
        const auto cosine = std::cos (omega);

        // RBJ Audio EQ Cookbook의 Peaking/Bell EQ 공식이다.
        const auto rawB0 = 1.0 + alpha * a;
        const auto rawB1 = -2.0 * cosine;
        const auto rawB2 = 1.0 - alpha * a;
        const auto rawA0 = 1.0 + alpha / a;
        const auto rawA1 = -2.0 * cosine;
        const auto rawA2 = 1.0 - alpha / a;

        // 모든 항을 a0로 나누면 현재 출력 y[n] 앞의 계수가 1이 되어
        // processSample()에서 다섯 계수만으로 계산할 수 있다.
        return {
            static_cast<float> (rawB0 / rawA0),
            static_cast<float> (rawB1 / rawA0),
            static_cast<float> (rawB2 / rawA0),
            static_cast<float> (rawA1 / rawA0),
            static_cast<float> (rawA2 / rawA0)
        };
    }

    void setCoefficients (BiquadCoefficients newCoefficients) noexcept
    {
        coefficients_ = newCoefficients;
    }

    float processSample (float currentInput) noexcept
    {
        // y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2]
        //                      - a1*y[n-1] - a2*y[n-2]
        const auto currentOutput = coefficients_.b0 * currentInput
                                   + coefficients_.b1 * previousInput1_
                                   + coefficients_.b2 * previousInput2_
                                   - coefficients_.a1 * previousOutput1_
                                   - coefficients_.a2 * previousOutput2_;

        // 다음 샘플을 계산할 수 있도록 현재 입력과 출력을 과거 상태로 옮긴다.
        previousInput2_ = previousInput1_;
        previousInput1_ = currentInput;
        previousOutput2_ = previousOutput1_;
        previousOutput1_ = currentOutput;

        return currentOutput;
    }

    void reset() noexcept
    {
        previousInput1_ = 0.0f;
        previousInput2_ = 0.0f;
        previousOutput1_ = 0.0f;
        previousOutput2_ = 0.0f;
    }

private:
    BiquadCoefficients coefficients_;
    float previousInput1_ = 0.0f;
    float previousInput2_ = 0.0f;
    float previousOutput1_ = 0.0f;
    float previousOutput2_ = 0.0f;
};

} // namespace eqlab
