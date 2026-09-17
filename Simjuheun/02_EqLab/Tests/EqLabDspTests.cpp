#include "DSP/PassthroughDsp.h"

#include <cmath>
#include <iostream>

namespace
{
bool nearlyEqual (float actual, float expected, float tolerance = 0.0001f)
{
    return std::abs (actual - expected) <= tolerance;
}
}

int main()
{
    // EQ를 연결하기 전 기준 impulse다. Pass-through라면 출력이 입력과 같아야 한다.
    const float input[] { 1.0f, 0.0f, 0.0f, 0.0f };
    float output[4] {};

    for (int sample = 0; sample < 4; ++sample)
        output[sample] = eqlab::PassthroughDsp::processSample (input[sample]);

    std::cout << "입력 impulse: [ 1, 0, 0, 0 ]\n";
    std::cout << "현재 출력:    [ " << output[0] << ", " << output[1]
              << ", " << output[2] << ", " << output[3] << " ]\n";

    for (int sample = 0; sample < 4; ++sample)
    {
        if (! nearlyEqual (output[sample], input[sample]))
        {
            std::cerr << "FAIL: Pass-through 출력이 입력과 다릅니다.\n";
            return 1;
        }
    }

    std::cout << "PASS: EqLab의 DSP 시작점은 올바른 Pass-through 상태입니다.\n";
    return 0;
}
