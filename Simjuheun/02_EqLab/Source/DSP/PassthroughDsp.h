#pragma once

namespace eqlab
{

// 아직 EQ를 연결하지 않은 기준 상태다.
// 입력 샘플을 전혀 바꾸지 않고 그대로 반환하므로 이후 필터 결과와 비교할 수 있다.
class PassthroughDsp
{
public:
    static float processSample (float inputSample) noexcept
    {
        return inputSample;
    }
};

} // namespace eqlab
