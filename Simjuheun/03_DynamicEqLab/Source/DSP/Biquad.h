#pragma once

#include <algorithm>
#include <cmath>

namespace dynamic_eq_lab
{

struct BiquadCoefficients
{
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

// Biquad의 계산식은 필터 종류가 달라져도 동일하다.
// Bell과 Band-pass의 차이는 이 상태 클래스가 아니라 다섯 계수에 있다.
class Biquad
{
public:
    void setCoefficients (BiquadCoefficients coefficients) noexcept
    {
        coefficients_ = coefficients;
    }

    double processSample (double input) noexcept
    {
        // Direct Form II Transposed:
        // 과거 샘플을 직접 보관하는 대신 같은 정보를 두 상태 z1, z2에 모아 둔다.
        const auto output = coefficients_.b0 * input + z1_;
        z1_ = coefficients_.b1 * input - coefficients_.a1 * output + z2_;
        z2_ = coefficients_.b2 * input - coefficients_.a2 * output;
        return output;
    }

    void reset() noexcept
    {
        z1_ = 0.0;
        z2_ = 0.0;
    }

private:
    BiquadCoefficients coefficients_ {};
    double z1_ = 0.0;
    double z2_ = 0.0;
};

class BiquadDesigner
{
public:
    static BiquadCoefficients makeBell (double sampleRate,
                                         double frequencyHz,
                                         double q,
                                         double gainDb) noexcept
    {
        const auto common = commonValues (sampleRate, frequencyHz, q);

        // Bell EQ에서 A는 사용자가 입력한 dB Gain을 계수 공식에 사용할 배율로 바꾼 값이다.
        const auto a = std::pow (10.0, gainDb / 40.0);
        const auto rawB0 = 1.0 + common.alpha * a;
        const auto rawB1 = -2.0 * common.cosine;
        const auto rawB2 = 1.0 - common.alpha * a;
        const auto rawA0 = 1.0 + common.alpha / a;
        const auto rawA1 = -2.0 * common.cosine;
        const auto rawA2 = 1.0 - common.alpha / a;

        return normalise (rawB0, rawB1, rawB2, rawA0, rawA1, rawA2);
    }

    static BiquadCoefficients makeBandPass (double sampleRate,
                                             double frequencyHz,
                                             double q) noexcept
    {
        const auto common = commonValues (sampleRate, frequencyHz, q);

        // 중심 주파수 부근만 감지 경로로 통과시킨다.
        // 이 신호는 출력에 섞지 않고 "선택 대역이 얼마나 큰가"를 재는 데만 사용한다.
        const auto rawB0 = common.alpha;
        const auto rawB1 = 0.0;
        const auto rawB2 = -common.alpha;
        const auto rawA0 = 1.0 + common.alpha;
        const auto rawA1 = -2.0 * common.cosine;
        const auto rawA2 = 1.0 - common.alpha;

        return normalise (rawB0, rawB1, rawB2, rawA0, rawA1, rawA2);
    }

    static double magnitudeDbAt (const BiquadCoefficients& c,
                                  double sampleRate,
                                  double frequencyHz) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        const auto omega = 2.0 * pi * frequencyHz / std::max (sampleRate, 1.0);
        const auto cos1 = std::cos (omega);
        const auto sin1 = std::sin (omega);
        const auto cos2 = std::cos (2.0 * omega);
        const auto sin2 = std::sin (2.0 * omega);

        const auto nr = c.b0 + c.b1 * cos1 + c.b2 * cos2;
        const auto ni = -c.b1 * sin1 - c.b2 * sin2;
        const auto dr = 1.0 + c.a1 * cos1 + c.a2 * cos2;
        const auto di = -c.a1 * sin1 - c.a2 * sin2;
        const auto magnitude = std::sqrt ((nr * nr + ni * ni)
                                          / std::max (dr * dr + di * di, 1.0e-24));
        return 20.0 * std::log10 (std::max (magnitude, 1.0e-12));
    }

private:
    struct CommonValues
    {
        double cosine = 1.0;
        double alpha = 0.0;
    };

    static CommonValues commonValues (double sampleRate,
                                       double frequencyHz,
                                       double q) noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        const auto safeRate = std::max (sampleRate, 1.0);
        const auto safeFrequency = std::clamp (frequencyHz, 1.0, safeRate * 0.49);
        const auto safeQ = std::max (q, 0.01);
        const auto omega = 2.0 * pi * safeFrequency / safeRate;

        return { std::cos (omega), std::sin (omega) / (2.0 * safeQ) };
    }

    static BiquadCoefficients normalise (double b0, double b1, double b2,
                                          double a0, double a1, double a2) noexcept
    {
        const auto inverseA0 = 1.0 / std::max (a0, 1.0e-12);
        return { b0 * inverseA0, b1 * inverseA0, b2 * inverseA0,
                 a1 * inverseA0, a2 * inverseA0 };
    }
};

} // namespace dynamic_eq_lab
