#pragma once

#include <algorithm>
#include <cmath>

namespace gainlab
{

// 이전 값에서 목표 값까지 일정한 속도로 이동시키는 선형 smoother다.
// currentValue_, targetValue_, samplesRemaining_이 블록 사이에도 유지돼야 하므로
// 이 클래스의 객체는 processBlock()의 지역변수가 아니라 Processor의 멤버로 둔다.
class LinearSmoother
{
public:
    void prepare (double sampleRate, double rampSeconds) noexcept
    {
        // 시간을 샘플 개수로 바꾼다.
        // 예: 48 kHz에서 20 ms라면 48,000 * 0.020 = 960 samples다.
        rampLengthSamples_ = std::max (1,
                                      static_cast<int> (std::round (sampleRate * rampSeconds)));
        samplesRemaining_ = 0;
        step_ = 0.0f;
    }

    void setCurrentAndTargetValue (float value) noexcept
    {
        // 재생 시작 시에는 불필요한 ramp가 생기지 않도록 현재값과 목표값을 같게 만든다.
        currentValue_ = value;
        targetValue_ = value;
        samplesRemaining_ = 0;
        step_ = 0.0f;
    }

    void setTargetValue (float newTarget) noexcept
    {
        // 같은 목표를 매 블록 전달받더라도 진행 중인 ramp를 다시 시작하지 않는다.
        if (std::abs (newTarget - targetValue_) <= 1.0e-8f)
            return;

        targetValue_ = newTarget;
        samplesRemaining_ = rampLengthSamples_;
        step_ = (targetValue_ - currentValue_)
                / static_cast<float> (samplesRemaining_);
    }

    float getNextValue() noexcept
    {
        if (samplesRemaining_ <= 0)
            return targetValue_;

        currentValue_ += step_;
        --samplesRemaining_;

        // 부동소수점 오차가 누적되지 않도록 마지막 샘플은 목표값에 정확히 맞춘다.
        if (samplesRemaining_ == 0)
            currentValue_ = targetValue_;

        return currentValue_;
    }

    float getCurrentValue() const noexcept { return currentValue_; }
    bool isSmoothing() const noexcept { return samplesRemaining_ > 0; }

private:
    float currentValue_ = 1.0f;
    float targetValue_ = 1.0f;
    float step_ = 0.0f;
    int rampLengthSamples_ = 1;
    int samplesRemaining_ = 0;
};

} // namespace gainlab
