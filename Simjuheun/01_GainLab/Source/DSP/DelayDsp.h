#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace gainlab
{

// 과거의 입력 샘플을 원형 버퍼에 보관했다가 지정한 시간 뒤에 꺼내는 Delay DSP다.
// 버퍼의 끝에 도달하면 다시 처음으로 돌아가므로 긴 재생 중에도 같은 메모리를 재사용한다.
class DelayDsp
{
public:
    void prepare (double sampleRate,
                  float maximumDelayMilliseconds,
                  int numberOfChannels)
    {
        sampleRate_ = std::max (1.0, sampleRate);
        maximumDelaySamples_ = std::max (
            1,
            static_cast<int> (std::ceil (sampleRate_
                                         * maximumDelayMilliseconds
                                         / 1000.0)));

        // +1은 소수점 단위의 delay 위치를 선형 보간할 때 필요한 여유 공간이다.
        const auto bufferLength = maximumDelaySamples_ + 1;
        delayBuffer_.assign (static_cast<std::size_t> (std::max (1, numberOfChannels)),
                             std::vector<float> (static_cast<std::size_t> (bufferLength),
                                                 0.0f));
        writePosition_ = 0;
    }

    void reset() noexcept
    {
        for (auto& channel : delayBuffer_)
            std::fill (channel.begin(), channel.end(), 0.0f);

        writePosition_ = 0;
    }

    float millisecondsToSamples (float milliseconds) const noexcept
    {
        const auto samples = milliseconds * static_cast<float> (sampleRate_) / 1000.0f;
        return std::clamp (samples, 1.0f, static_cast<float> (maximumDelaySamples_));
    }

    float processSample (int channel,
                         float inputSample,
                         float delayInSamples,
                         float feedback) noexcept
    {
        if (delayBuffer_.empty()
            || channel < 0
            || channel >= static_cast<int> (delayBuffer_.size()))
            return 0.0f;

        auto& channelBuffer = delayBuffer_[static_cast<std::size_t> (channel)];
        const auto bufferLength = static_cast<int> (channelBuffer.size());
        const auto safeDelay = std::clamp (delayInSamples,
                                           1.0f,
                                           static_cast<float> (maximumDelaySamples_));

        // 현재 쓰는 위치보다 delayInSamples만큼 과거로 이동해 Wet 샘플을 읽는다.
        auto readPosition = static_cast<float> (writePosition_) - safeDelay;
        while (readPosition < 0.0f)
            readPosition += static_cast<float> (bufferLength);

        const auto indexA = static_cast<int> (std::floor (readPosition));
        const auto indexB = (indexA + 1) % bufferLength;
        const auto fraction = readPosition - static_cast<float> (indexA);

        // Delay Time이 소수점 샘플 위치를 가리켜도 부드럽게 읽도록 이웃 샘플을 보간한다.
        const auto delayedSample = channelBuffer[static_cast<std::size_t> (indexA)]
                                   + (channelBuffer[static_cast<std::size_t> (indexB)]
                                      - channelBuffer[static_cast<std::size_t> (indexA)])
                                         * fraction;

        // 지연된 신호 일부를 다시 버퍼에 넣으면 같은 소리가 반복되는 Feedback이 된다.
        const auto safeFeedback = std::clamp (feedback, 0.0f, 0.95f);
        channelBuffer[static_cast<std::size_t> (writePosition_)]
            = inputSample + delayedSample * safeFeedback;

        return delayedSample;
    }

    void advance() noexcept
    {
        if (delayBuffer_.empty())
            return;

        writePosition_ = (writePosition_ + 1)
                         % static_cast<int> (delayBuffer_.front().size());
    }

private:
    std::vector<std::vector<float>> delayBuffer_;
    double sampleRate_ = 44100.0;
    int maximumDelaySamples_ = 1;
    int writePosition_ = 0;
};

} // namespace gainlab
