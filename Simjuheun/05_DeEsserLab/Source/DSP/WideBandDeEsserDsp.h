#pragma once

#include "Biquad.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace deesser_lab
{

struct WideBandDeEsserParameters
{
    double detectorFrequencyHz = 7000.0;
    double detectorQ = 1.2;
    double thresholdDb = -30.0;
    double ratio = 4.0;
    double maxReductionDb = 10.0;
    double attackMs = 1.0;
    double releaseMs = 80.0;
    bool detectorListen = false;
};

// 치찰음 대역을 감지하되, 동작할 때는 원본 전체를 줄이는 Wide-band De-esser다.
class WideBandDeEsserDsp
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int numChannels) noexcept
    {
        sampleRate_ = std::max (sampleRate, 1.0);
        numChannels_ = std::clamp (numChannels, 1, maxChannels);
        updateDetectorCoefficients();
        updateTimeConstants();
        reset();
    }

    void setParameters (WideBandDeEsserParameters parameters) noexcept
    {
        parameters.detectorFrequencyHz = std::clamp (
            parameters.detectorFrequencyHz, 1000.0, sampleRate_ * 0.45);
        parameters.detectorQ = std::clamp (parameters.detectorQ, 0.2, 12.0);
        parameters.thresholdDb = std::clamp (parameters.thresholdDb, -100.0, 0.0);
        parameters.ratio = std::max (parameters.ratio, 1.0);
        parameters.maxReductionDb = std::max (parameters.maxReductionDb, 0.0);
        parameters.attackMs = std::max (parameters.attackMs, 0.01);
        parameters.releaseMs = std::max (parameters.releaseMs, 0.01);

        constexpr double epsilon = 1.0e-9;
        const auto detectorChanged =
            std::abs (parameters.detectorFrequencyHz - parameters_.detectorFrequencyHz) > epsilon
            || std::abs (parameters.detectorQ - parameters_.detectorQ) > epsilon;
        const auto timeChanged =
            std::abs (parameters.attackMs - parameters_.attackMs) > epsilon
            || std::abs (parameters.releaseMs - parameters_.releaseMs) > epsilon;
        parameters_ = parameters;

        if (detectorChanged)
            updateDetectorCoefficients();
        if (timeChanged)
            updateTimeConstants();
    }

    static double calculateReductionDb (double detectorDb,
                                        double thresholdDb,
                                        double ratio,
                                        double maxReductionDb) noexcept
    {
        const auto excessDb = std::max (detectorDb - thresholdDb, 0.0);
        const auto safeRatio = std::max (ratio, 1.0);
        return std::clamp (excessDb * (1.0 - 1.0 / safeRatio),
                           0.0, std::max (maxReductionDb, 0.0));
    }

    static double dbToLinear (double db) noexcept
    {
        return std::pow (10.0, db / 20.0);
    }

    void processSampleFrame (float* samples, int numChannels) noexcept
    {
        const auto channels = std::clamp (numChannels, 1, numChannels_);
        std::array<double, maxChannels> detectorSamples {};

        // 감지 경로: 각 채널에서 치찰음 대역만 추출한다.
        double detectorPeak = 0.0;
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto index = static_cast<std::size_t> (channel);
            detectorSamples[index] = detectorFilters_[index].processSample (samples[channel]);
            detectorPeak = std::max (detectorPeak, std::abs (detectorSamples[index]));
        }

        const auto coefficient = detectorPeak > envelope_ ? attackCoefficient_
                                                           : releaseCoefficient_;
        envelope_ = detectorPeak + (envelope_ - detectorPeak) * coefficient;
        detectorDb_ = 20.0 * std::log10 (std::max (envelope_, 1.0e-9));
        reductionDb_ = calculateReductionDb (
            detectorDb_, parameters_.thresholdDb, parameters_.ratio,
            parameters_.maxReductionDb);

        if (parameters_.detectorListen)
        {
            // 사용자가 감지기가 듣는 치찰음 대역을 직접 확인하는 모드다.
            for (int channel = 0; channel < channels; ++channel)
                samples[channel] = static_cast<float> (
                    detectorSamples[static_cast<std::size_t> (channel)]);
            return;
        }

        // Wide-band의 핵심: 감지는 고역에서 했지만 Gain은 원본 전체에 곱한다.
        const auto gain = dbToLinear (-reductionDb_);
        for (int channel = 0; channel < channels; ++channel)
            samples[channel] = static_cast<float> (samples[channel] * gain);
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
        envelope_ = 0.0;
        detectorDb_ = -180.0;
        reductionDb_ = 0.0;
    }

    double detectorDb() const noexcept { return detectorDb_; }
    double reductionDb() const noexcept { return reductionDb_; }

private:
    void updateDetectorCoefficients() noexcept
    {
        const auto coefficients = makeBandPass (
            sampleRate_, parameters_.detectorFrequencyHz, parameters_.detectorQ);
        for (auto& filter : detectorFilters_)
            filter.setCoefficients (coefficients);
    }

    void updateTimeConstants() noexcept
    {
        attackCoefficient_ = timeCoefficient (parameters_.attackMs);
        releaseCoefficient_ = timeCoefficient (parameters_.releaseMs);
    }

    double timeCoefficient (double milliseconds) const noexcept
    {
        return std::exp (-1.0 / (0.001 * milliseconds * sampleRate_));
    }

    double sampleRate_ = 48000.0;
    int numChannels_ = 2;
    WideBandDeEsserParameters parameters_ {};
    std::array<Biquad, maxChannels> detectorFilters_ {};
    double attackCoefficient_ = 0.0;
    double releaseCoefficient_ = 0.0;
    double envelope_ = 0.0;
    double detectorDb_ = -180.0;
    double reductionDb_ = 0.0;
};

} // namespace deesser_lab
