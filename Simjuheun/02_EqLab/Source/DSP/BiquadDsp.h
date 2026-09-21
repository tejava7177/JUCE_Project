#pragma once

#include <algorithm>
#include <cmath>

namespace eqlab
{

// 사용자가 선택할 수 있는 Biquad 필터 모드다.
enum class FilterType
{
    bell = 0,
    lowPass,
    highPass,
    lowShelf,
    highShelf,
    notch
};

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

// 필터 종류에 맞는 계수를 만들고, 같은 Biquad 계산식으로 샘플을 처리한다.
// 필터 종류의 차이는 processSample()이 아니라 계수 공식에 있다.
class BiquadDsp
{
public:
    static BiquadCoefficients makeCoefficients (FilterType type,
                                                 double sampleRate,
                                                 float frequencyHz,
                                                 float gainDb,
                                                 float q,
                                                 float shelfSlope) noexcept
    {
        switch (type)
        {
            case FilterType::lowPass:
                return makeLowPassCoefficients (sampleRate, frequencyHz, q);
            case FilterType::highPass:
                return makeHighPassCoefficients (sampleRate, frequencyHz, q);
            case FilterType::lowShelf:
                return makeLowShelfCoefficients (
                    sampleRate, frequencyHz, gainDb, shelfSlope);
            case FilterType::highShelf:
                return makeHighShelfCoefficients (
                    sampleRate, frequencyHz, gainDb, shelfSlope);
            case FilterType::notch:
                return makeNotchCoefficients (sampleRate, frequencyHz, q);
            case FilterType::bell:
            default:
                return makeBellCoefficients (sampleRate, frequencyHz, gainDb, q);
        }
    }

    static BiquadCoefficients makeBellCoefficients (double sampleRate,
                                                     float frequencyHz,
                                                     float gainDb,
                                                     float q) noexcept
    {
        const auto common = makeCommonValues (sampleRate, frequencyHz, q);

        // Bell은 Gain을 A라는 값으로 바꿔 분자와 분모에 반대 방향으로 적용한다.
        const auto a = std::pow (10.0, static_cast<double> (gainDb) / 40.0);

        const auto rawB0 = 1.0 + common.alpha * a;
        const auto rawB1 = -2.0 * common.cosine;
        const auto rawB2 = 1.0 - common.alpha * a;
        const auto rawA0 = 1.0 + common.alpha / a;
        const auto rawA1 = -2.0 * common.cosine;
        const auto rawA2 = 1.0 - common.alpha / a;

        return normalise (rawB0, rawB1, rawB2, rawA0, rawA1, rawA2);
    }

    static BiquadCoefficients makeLowPassCoefficients (double sampleRate,
                                                        float cutoffHz,
                                                        float q) noexcept
    {
        const auto common = makeCommonValues (sampleRate, cutoffHz, q);

        // cos(omega)가 1에 가까운 저주파는 통과시키고,
        // cutoff보다 높은 주파수는 점점 상쇄시키는 Low-pass 공식이다.
        const auto rawB0 = (1.0 - common.cosine) * 0.5;
        const auto rawB1 = 1.0 - common.cosine;
        const auto rawB2 = (1.0 - common.cosine) * 0.5;
        const auto rawA0 = 1.0 + common.alpha;
        const auto rawA1 = -2.0 * common.cosine;
        const auto rawA2 = 1.0 - common.alpha;

        return normalise (rawB0, rawB1, rawB2, rawA0, rawA1, rawA2);
    }

    static BiquadCoefficients makeHighPassCoefficients (double sampleRate,
                                                         float cutoffHz,
                                                         float q) noexcept
    {
        const auto common = makeCommonValues (sampleRate, cutoffHz, q);

        // Low-pass와 분모(a 계수)는 같지만 분자(b 계수)의 부호가 달라서,
        // 저주파는 상쇄되고 cutoff보다 높은 주파수는 통과한다.
        const auto rawB0 = (1.0 + common.cosine) * 0.5;
        const auto rawB1 = -(1.0 + common.cosine);
        const auto rawB2 = (1.0 + common.cosine) * 0.5;
        const auto rawA0 = 1.0 + common.alpha;
        const auto rawA1 = -2.0 * common.cosine;
        const auto rawA2 = 1.0 - common.alpha;

        return normalise (rawB0, rawB1, rawB2, rawA0, rawA1, rawA2);
    }

    static BiquadCoefficients makeLowShelfCoefficients (double sampleRate,
                                                         float frequencyHz,
                                                         float gainDb,
                                                         float slope) noexcept
    {
        const auto common = makeShelfValues (
            sampleRate, frequencyHz, gainDb, slope);

        // Low Shelf는 저주파 쪽이 Gain A^2에 도달하고 고주파는 0 dB로 돌아온다.
        // Slope는 두 평평한 영역을 연결하는 전환 구간의 경사를 결정한다.
        const auto rawB0 = common.a * ((common.a + 1.0)
                                      - (common.a - 1.0) * common.cosine
                                      + 2.0 * common.sqrtA * common.alpha);
        const auto rawB1 = 2.0 * common.a * ((common.a - 1.0)
                                            - (common.a + 1.0) * common.cosine);
        const auto rawB2 = common.a * ((common.a + 1.0)
                                      - (common.a - 1.0) * common.cosine
                                      - 2.0 * common.sqrtA * common.alpha);
        const auto rawA0 = (common.a + 1.0)
                           + (common.a - 1.0) * common.cosine
                           + 2.0 * common.sqrtA * common.alpha;
        const auto rawA1 = -2.0 * ((common.a - 1.0)
                                  + (common.a + 1.0) * common.cosine);
        const auto rawA2 = (common.a + 1.0)
                           + (common.a - 1.0) * common.cosine
                           - 2.0 * common.sqrtA * common.alpha;

        return normalise (rawB0, rawB1, rawB2, rawA0, rawA1, rawA2);
    }

    static BiquadCoefficients makeHighShelfCoefficients (double sampleRate,
                                                          float frequencyHz,
                                                          float gainDb,
                                                          float slope) noexcept
    {
        const auto common = makeShelfValues (
            sampleRate, frequencyHz, gainDb, slope);

        // High Shelf는 Low Shelf의 거울 형태로, 고주파 쪽이 Gain A^2에 도달한다.
        const auto rawB0 = common.a * ((common.a + 1.0)
                                      + (common.a - 1.0) * common.cosine
                                      + 2.0 * common.sqrtA * common.alpha);
        const auto rawB1 = -2.0 * common.a * ((common.a - 1.0)
                                             + (common.a + 1.0) * common.cosine);
        const auto rawB2 = common.a * ((common.a + 1.0)
                                      + (common.a - 1.0) * common.cosine
                                      - 2.0 * common.sqrtA * common.alpha);
        const auto rawA0 = (common.a + 1.0)
                           - (common.a - 1.0) * common.cosine
                           + 2.0 * common.sqrtA * common.alpha;
        const auto rawA1 = 2.0 * ((common.a - 1.0)
                                 - (common.a + 1.0) * common.cosine);
        const auto rawA2 = (common.a + 1.0)
                           - (common.a - 1.0) * common.cosine
                           - 2.0 * common.sqrtA * common.alpha;

        return normalise (rawB0, rawB1, rawB2, rawA0, rawA1, rawA2);
    }

    static BiquadCoefficients makeNotchCoefficients (double sampleRate,
                                                      float frequencyHz,
                                                      float q) noexcept
    {
        const auto common = makeCommonValues (sampleRate, frequencyHz, q);

        // 분자의 b 계수는 선택한 Frequency에서 현재 입력과 과거 입력이
        // 정확히 반대 방향으로 겹치게 만들어 중심 주파수를 상쇄한다.
        // Q는 분모의 alpha를 바꿔 상쇄되는 홈의 폭을 결정한다.
        const auto rawB0 = 1.0;
        const auto rawB1 = -2.0 * common.cosine;
        const auto rawB2 = 1.0;
        const auto rawA0 = 1.0 + common.alpha;
        const auto rawA1 = -2.0 * common.cosine;
        const auto rawA2 = 1.0 - common.alpha;

        return normalise (rawB0, rawB1, rawB2, rawA0, rawA1, rawA2);
    }

    // GUI 그래프에서 특정 주파수가 몇 dB로 변하는지 계산한다.
    // 실제 오디오 처리와 같은 계수를 사용하므로 화면의 곡선과 DSP가 일치한다.
    static double magnitudeDbAt (const BiquadCoefficients& coefficients,
                                 double sampleRate,
                                 double frequencyHz) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        const auto omega = 2.0 * pi * frequencyHz / std::max (1.0, sampleRate);
        const auto cosine1 = std::cos (omega);
        const auto sine1 = std::sin (omega);
        const auto cosine2 = std::cos (2.0 * omega);
        const auto sine2 = std::sin (2.0 * omega);

        const auto numeratorReal = coefficients.b0
                                   + coefficients.b1 * cosine1
                                   + coefficients.b2 * cosine2;
        const auto numeratorImaginary = -coefficients.b1 * sine1
                                        - coefficients.b2 * sine2;
        const auto denominatorReal = 1.0
                                     + coefficients.a1 * cosine1
                                     + coefficients.a2 * cosine2;
        const auto denominatorImaginary = -coefficients.a1 * sine1
                                          - coefficients.a2 * sine2;

        const auto numeratorPower = numeratorReal * numeratorReal
                                    + numeratorImaginary * numeratorImaginary;
        const auto denominatorPower = denominatorReal * denominatorReal
                                      + denominatorImaginary * denominatorImaginary;
        const auto magnitude = std::sqrt (numeratorPower
                                          / std::max (denominatorPower, 1.0e-24));

        return 20.0 * std::log10 (std::max (magnitude, 1.0e-12));
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
    struct CommonValues
    {
        double alpha;
        double cosine;
    };

    struct ShelfValues
    {
        double a;
        double alpha;
        double cosine;
        double sqrtA;
    };

    static CommonValues makeCommonValues (double sampleRate,
                                           float frequencyHz,
                                           float q) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        const auto safeSampleRate = std::max (1.0, sampleRate);
        const auto maximumFrequency = static_cast<float> (safeSampleRate * 0.499);
        const auto safeFrequency = std::clamp (frequencyHz, 1.0f, maximumFrequency);
        const auto safeQ = std::max (0.01f, q);
        const auto omega = 2.0 * pi * static_cast<double> (safeFrequency)
                           / safeSampleRate;

        return { std::sin (omega) / (2.0 * static_cast<double> (safeQ)),
                 std::cos (omega) };
    }

    static ShelfValues makeShelfValues (double sampleRate,
                                         float frequencyHz,
                                         float gainDb,
                                         float slope) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        const auto safeSampleRate = std::max (1.0, sampleRate);
        const auto maximumFrequency = static_cast<float> (safeSampleRate * 0.499);
        const auto safeFrequency = std::clamp (frequencyHz, 1.0f, maximumFrequency);
        const auto safeSlope = std::clamp (slope, 0.1f, 1.0f);
        const auto omega = 2.0 * pi * static_cast<double> (safeFrequency)
                           / safeSampleRate;
        const auto a = std::pow (10.0, static_cast<double> (gainDb) / 40.0);

        // S=1은 가장 급하면서 단조로운 표준 Shelf이고,
        // S가 작아질수록 두 평평한 영역을 더 넓고 완만하게 연결한다.
        const auto alpha = std::sin (omega) * 0.5
                           * std::sqrt ((a + 1.0 / a)
                                            * (1.0 / safeSlope - 1.0)
                                        + 2.0);

        return { a, alpha, std::cos (omega), std::sqrt (a) };
    }

    static BiquadCoefficients normalise (double rawB0,
                                         double rawB1,
                                         double rawB2,
                                         double rawA0,
                                         double rawA1,
                                         double rawA2) noexcept
    {
        // a0로 모두 나누면 출력 식의 a0가 1이 되어 다섯 계수만 저장하면 된다.
        return {
            static_cast<float> (rawB0 / rawA0),
            static_cast<float> (rawB1 / rawA0),
            static_cast<float> (rawB2 / rawA0),
            static_cast<float> (rawA1 / rawA0),
            static_cast<float> (rawA2 / rawA0)
        };
    }

    BiquadCoefficients coefficients_;
    float previousInput1_ = 0.0f;
    float previousInput2_ = 0.0f;
    float previousOutput1_ = 0.0f;
    float previousOutput2_ = 0.0f;
};

} // namespace eqlab
