#pragma once

#include "Biquad.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace dynamic_eq_lab
{

struct DynamicBellParameters
{
    double frequencyHz = 1000.0;
    double q = 2.0;
    double thresholdDb = -24.0;
    double ratio = 4.0;
    double maxReductionDb = 6.0;
    double attackMs = 10.0;
    double releaseMs = 120.0;
};

// 하향식 Dynamic Bell EQ의 전체 학습용 구현이다.
// 1) Band-pass로 선택 대역만 듣는다.
// 2) Envelope가 Threshold를 얼마나 넘었는지 계산한다.
// 3) 그 결과를 음수 Bell Gain으로 바꿔 원본 신호에 적용한다.
class DynamicBellDsp
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int numChannels) noexcept
    {
        sampleRate_ = std::max (sampleRate, 1.0);
        numChannels_ = std::clamp (numChannels, 1, maxChannels);
        reset();
        updateTimeConstants();
        updateDetectorCoefficients();
    }

    void setParameters (DynamicBellParameters parameters) noexcept
    {
        parameters.frequencyHz = std::clamp (
            parameters.frequencyHz, 20.0, sampleRate_ * 0.45);
        parameters.q = std::clamp (parameters.q, 0.1, 20.0);
        parameters.thresholdDb = std::clamp (parameters.thresholdDb, -100.0, 0.0);
        parameters.ratio = std::max (parameters.ratio, 1.0);
        parameters.maxReductionDb = std::max (parameters.maxReductionDb, 0.0);
        parameters.attackMs = std::max (parameters.attackMs, 0.01);
        parameters.releaseMs = std::max (parameters.releaseMs, 0.01);

        constexpr double changeEpsilon = 1.0e-9;
        const auto detectorShapeChanged =
            std::abs (parameters.frequencyHz - parameters_.frequencyHz) > changeEpsilon
            || std::abs (parameters.q - parameters_.q) > changeEpsilon;
        const auto timeChanged =
            std::abs (parameters.attackMs - parameters_.attackMs) > changeEpsilon
            || std::abs (parameters.releaseMs - parameters_.releaseMs) > changeEpsilon;
        parameters_ = parameters;

        if (detectorShapeChanged)
            updateDetectorCoefficients();
        if (timeChanged)
            updateTimeConstants();
    }

    // Ratio에 따른 이상적인 Gain Reduction을 계산한다.
    // 초과량이 12 dB이고 Ratio가 4:1이면 12 * (1 - 1/4) = 9 dB다.
    static double calculateTargetReductionDb (double detectorDb,
                                               double thresholdDb,
                                               double ratio,
                                               double maxReductionDb) noexcept
    {
        const auto excessDb = std::max (detectorDb - thresholdDb, 0.0);
        const auto safeRatio = std::max (ratio, 1.0);
        const auto compressedDb = excessDb * (1.0 - 1.0 / safeRatio);
        return std::clamp (compressedDb, 0.0, std::max (maxReductionDb, 0.0));
    }

    void processSampleFrame (float* samples, int numChannels) noexcept
    {
        const auto channels = std::clamp (numChannels, 1, numChannels_);

        // 스테레오 어느 한쪽에서라도 문제가 크면 양쪽을 같은 양만큼 줄인다.
        // 이 링크가 없으면 좌우 Gain이 달라져 음상이 흔들릴 수 있다.
        double detectorPeak = 0.0;
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto detected = detectorFilters_[static_cast<std::size_t> (channel)]
                                      .processSample (samples[channel]);
            detectorPeak = std::max (detectorPeak, std::abs (detected));
        }

        // 갑작스러운 레벨 변화를 그대로 쓰지 않고 Attack/Release 속도로 따라간다.
        const auto coefficient = detectorPeak > envelope_ ? attackCoefficient_
                                                           : releaseCoefficient_;
        envelope_ = coefficient * envelope_ + (1.0 - coefficient) * detectorPeak;
        detectorDb_ = 20.0 * std::log10 (std::max (envelope_, 1.0e-9));

        reductionDb_ = calculateTargetReductionDb (
            detectorDb_, parameters_.thresholdDb, parameters_.ratio,
            parameters_.maxReductionDb);

        // 일반 EQ와 같은 Bell 공식이지만 Gain이 고정 노브 값이 아니라
        // 감지 결과인 -reductionDb_로 매 샘플 부드럽게 바뀐다.
        const auto bell = BiquadDesigner::makeBell (
            sampleRate_, parameters_.frequencyHz, parameters_.q, -reductionDb_);

        for (int channel = 0; channel < channels; ++channel)
        {
            auto& filter = audioFilters_[static_cast<std::size_t> (channel)];
            filter.setCoefficients (bell);
            samples[channel] = static_cast<float> (filter.processSample (samples[channel]));
        }
    }

    void processBlock (float* const* channels, int numChannels, int numSamples) noexcept
    {
        std::array<float, maxChannels> frame {};
        const auto channelCount = std::clamp (numChannels, 1, numChannels_);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int channel = 0; channel < channelCount; ++channel)
                frame[static_cast<std::size_t> (channel)] = channels[channel][sample];

            processSampleFrame (frame.data(), channelCount);

            for (int channel = 0; channel < channelCount; ++channel)
                channels[channel][sample] = frame[static_cast<std::size_t> (channel)];
        }
    }

    void reset() noexcept
    {
        for (auto& filter : detectorFilters_)
            filter.reset();
        for (auto& filter : audioFilters_)
            filter.reset();
        envelope_ = 0.0;
        detectorDb_ = -180.0;
        reductionDb_ = 0.0;
    }

    double detectorDb() const noexcept { return detectorDb_; }
    double reductionDb() const noexcept { return reductionDb_; }
    const DynamicBellParameters& parameters() const noexcept { return parameters_; }

private:
    void updateDetectorCoefficients() noexcept
    {
        const auto coefficients = BiquadDesigner::makeBandPass (
            sampleRate_, parameters_.frequencyHz, parameters_.q);
        for (auto& filter : detectorFilters_)
            filter.setCoefficients (coefficients);
    }

    void updateTimeConstants() noexcept
    {
        // 한 샘플마다 이전 값을 얼마만큼 남길지 정하는 one-pole 계수다.
        attackCoefficient_ = timeCoefficient (parameters_.attackMs);
        releaseCoefficient_ = timeCoefficient (parameters_.releaseMs);
    }

    double timeCoefficient (double milliseconds) const noexcept
    {
        return std::exp (-1.0 / (0.001 * milliseconds * sampleRate_));
    }

    double sampleRate_ = 48000.0;
    int numChannels_ = 2;
    DynamicBellParameters parameters_ {};
    std::array<Biquad, maxChannels> detectorFilters_ {};
    std::array<Biquad, maxChannels> audioFilters_ {};
    double attackCoefficient_ = 0.0;
    double releaseCoefficient_ = 0.0;
    double envelope_ = 0.0;
    double detectorDb_ = -180.0;
    double reductionDb_ = 0.0;
};

} // namespace dynamic_eq_lab
