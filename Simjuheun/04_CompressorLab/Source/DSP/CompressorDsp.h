#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace compressor_lab
{

struct CompressorParameters
{
    double thresholdDb = -18.0;
    double ratio = 4.0;
    double attackMs = 10.0;
    double releaseMs = 120.0;
    double makeupDb = 0.0;
};

// 가장 기본적인 Peak Compressor다.
// 필터 없이 전체 신호의 크기를 감지하고, 계산한 Gain을 모든 샘플에 곱한다.
class CompressorDsp
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int numChannels) noexcept
    {
        sampleRate_ = std::max (sampleRate, 1.0);
        numChannels_ = std::clamp (numChannels, 1, maxChannels);
        updateTimeConstants();
        reset();
    }

    void setParameters (CompressorParameters parameters) noexcept
    {
        parameters.thresholdDb = std::clamp (parameters.thresholdDb, -100.0, 0.0);
        parameters.ratio = std::max (parameters.ratio, 1.0);
        parameters.attackMs = std::max (parameters.attackMs, 0.01);
        parameters.releaseMs = std::max (parameters.releaseMs, 0.01);
        parameters.makeupDb = std::clamp (parameters.makeupDb, -24.0, 24.0);

        constexpr double epsilon = 1.0e-9;
        const auto timesChanged = std::abs (parameters.attackMs - parameters_.attackMs) > epsilon
                                  || std::abs (parameters.releaseMs - parameters_.releaseMs) > epsilon;
        parameters_ = parameters;

        if (timesChanged)
            updateTimeConstants();
    }

    // Hard-knee Gain Computer.
    // Threshold 아래에서는 0 dB, 위에서는 Ratio가 요구하는 만큼 줄인다.
    static double calculateReductionDb (double inputDb,
                                        double thresholdDb,
                                        double ratio) noexcept
    {
        const auto excessDb = std::max (inputDb - thresholdDb, 0.0);
        const auto safeRatio = std::max (ratio, 1.0);
        return excessDb * (1.0 - 1.0 / safeRatio);
    }

    static double dbToLinear (double db) noexcept
    {
        return std::pow (10.0, db / 20.0);
    }

    void processSampleFrame (float* samples, int numChannels) noexcept
    {
        const auto channels = std::clamp (numChannels, 1, numChannels_);

        // 좌우 중 더 큰 Peak를 사용해 양쪽에 같은 Gain을 건다.
        // 그렇지 않으면 한쪽만 눌리면서 스테레오 중심이 흔들릴 수 있다.
        double peak = 0.0;
        for (int channel = 0; channel < channels; ++channel)
            peak = std::max (peak, std::abs (static_cast<double> (samples[channel])));

        // 순간 Peak를 Attack/Release 속도로 따라가는 Envelope Follower다.
        const auto envelopeCoefficient = peak > envelope_ ? attackCoefficient_
                                                           : releaseCoefficient_;
        envelope_ = peak + (envelope_ - peak) * envelopeCoefficient;
        inputDb_ = 20.0 * std::log10 (std::max (envelope_, 1.0e-9));

        reductionDb_ = calculateReductionDb (
            inputDb_, parameters_.thresholdDb, parameters_.ratio);

        // Compression으로 줄인 dB와 Makeup으로 다시 올릴 dB를 한 번에
        // 선형 배율로 바꾼다. 예: -6 dB + Makeup 2 dB = 최종 -4 dB.
        const auto outputGain = dbToLinear (parameters_.makeupDb - reductionDb_);
        for (int channel = 0; channel < channels; ++channel)
            samples[channel] = static_cast<float> (samples[channel] * outputGain);
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
        envelope_ = 0.0;
        inputDb_ = -180.0;
        reductionDb_ = 0.0;
    }

    double inputDb() const noexcept { return inputDb_; }
    double reductionDb() const noexcept { return reductionDb_; }

private:
    void updateTimeConstants() noexcept
    {
        attackCoefficient_ = timeCoefficient (parameters_.attackMs);
        releaseCoefficient_ = timeCoefficient (parameters_.releaseMs);
    }

    double timeCoefficient (double milliseconds) const noexcept
    {
        // ms를 "한 샘플마다 이전 값을 얼마나 기억할지"라는 계수로 바꾼다.
        return std::exp (-1.0 / (0.001 * milliseconds * sampleRate_));
    }

    double sampleRate_ = 48000.0;
    int numChannels_ = 2;
    CompressorParameters parameters_ {};
    double attackCoefficient_ = 0.0;
    double releaseCoefficient_ = 0.0;
    double envelope_ = 0.0;
    double inputDb_ = -180.0;
    double reductionDb_ = 0.0;
};

} // namespace compressor_lab
