#pragma once

#include <algorithm>
#include <cmath>

namespace deesser_lab
{

struct BiquadCoefficients
{
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

class Biquad
{
public:
    void setCoefficients (BiquadCoefficients coefficients) noexcept
    {
        coefficients_ = coefficients;
    }

    double processSample (double input) noexcept
    {
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

inline BiquadCoefficients makeBandPass (double sampleRate,
                                        double frequencyHz,
                                        double q) noexcept
{
    constexpr double pi = 3.14159265358979323846;
    const auto safeRate = std::max (sampleRate, 1.0);
    const auto safeFrequency = std::clamp (frequencyHz, 20.0, safeRate * 0.45);
    const auto safeQ = std::max (q, 0.1);
    const auto omega = 2.0 * pi * safeFrequency / safeRate;
    const auto cosine = std::cos (omega);
    const auto alpha = std::sin (omega) / (2.0 * safeQ);

    // 중심 주파수에서 0 dB가 되는 RBJ Band-pass다.
    // 출력에는 쓰지 않고 치찰음 감지용 Sidechain 신호를 만드는 데 사용한다.
    const auto a0 = 1.0 + alpha;
    return { alpha / a0,
             0.0,
             -alpha / a0,
             (-2.0 * cosine) / a0,
             (1.0 - alpha) / a0 };
}

inline double magnitudeDbAt (const BiquadCoefficients& c,
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

} // namespace deesser_lab
