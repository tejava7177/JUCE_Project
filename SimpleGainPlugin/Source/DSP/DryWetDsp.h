#pragma once

#include <algorithm>

namespace gainlab
{

// 처리 전 신호(Dry)와 처리 후 신호(Wet)를 섞는 공통 DSP다.
// Mix 0.0은 완전한 Dry, 0.5는 절반씩, 1.0은 완전한 Wet을 의미한다.
class DryWetDsp
{
public:
    static float percentToProportion (float mixPercent) noexcept
    {
        // UI는 0~100%가 읽기 편하지만 DSP 계산에는 0.0~1.0 비율을 사용한다.
        return std::clamp (mixPercent / 100.0f, 0.0f, 1.0f);
    }

    static float mixSample (float drySample,
                            float wetSample,
                            float wetProportion) noexcept
    {
        const auto wet = std::clamp (wetProportion, 0.0f, 1.0f);
        const auto dry = 1.0f - wet;

        return drySample * dry + wetSample * wet;
    }
};

} // namespace gainlab
