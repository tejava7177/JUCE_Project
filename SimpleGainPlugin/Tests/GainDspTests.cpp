#include "DSP/GainDsp.h"
#include "DSP/LinearSmoother.h"

#include <cmath>
#include <iostream>

namespace
{
bool nearlyEqual (float actual, float expected, float tolerance = 0.0001f)
{
    return std::abs (actual - expected) <= tolerance;
}

void printSamples (const char* label, const float* samples, int count)
{
    std::cout << label << " [ ";

    for (int i = 0; i < count; ++i)
        std::cout << samples[i] << (i + 1 == count ? " " : ", ");

    std::cout << "]\n";
}
} // namespace

int main()
{
    // DAW 대신 테스트 코드가 직접 준비한 가짜 오디오 샘플이다.
    float samples[] { 1.0f, 0.5f, -0.5f, -1.0f };
    constexpr int sampleCount = 4;

    // 진폭을 정확히 절반으로 만드는 dB 값이다.
    constexpr float gainDb = -6.0205999f;
    const auto linearGain = gainlab::GainDsp::decibelsToLinear (gainDb);

    printSamples ("입력 샘플:", samples, sampleCount);
    std::cout << "Gain: " << gainDb << " dB -> " << linearGain << "배\n";

    gainlab::GainDsp::applyGain (samples, sampleCount, linearGain);
    printSamples ("출력 샘플:", samples, sampleCount);

    const float expected[] { 0.5f, 0.25f, -0.25f, -0.5f };

    for (int i = 0; i < sampleCount; ++i)
    {
        if (! nearlyEqual (samples[i], expected[i]))
        {
            std::cerr << "FAIL: " << i << "번째 샘플이 예상값과 다릅니다.\n";
            return 1;
        }
    }

    // 0 dB가 정확히 1.0배인지도 별도로 확인한다.
    if (! nearlyEqual (gainlab::GainDsp::decibelsToLinear (0.0f), 1.0f))
    {
        std::cerr << "FAIL: 0 dB 변환 결과가 1.0이 아닙니다.\n";
        return 1;
    }

    // Smoothing 상태가 두 개의 오디오 블록 사이에서도 이어지는지 확인한다.
    // 테스트를 보기 쉽게 1,000 Hz * 0.004초 = 4 samples로 설정한다.
    gainlab::LinearSmoother smoother;
    smoother.prepare (1000.0, 0.004);
    smoother.setCurrentAndTargetValue (1.0f);
    smoother.setTargetValue (0.5f);

    float smoothingBlock1[] { smoother.getNextValue(), smoother.getNextValue() };
    float smoothingBlock2[] { smoother.getNextValue(), smoother.getNextValue() };

    printSamples ("Smoothing block 1:", smoothingBlock1, 2);
    printSamples ("Smoothing block 2:", smoothingBlock2, 2);

    const float expectedSmoothing[] { 0.875f, 0.75f, 0.625f, 0.5f };
    const float actualSmoothing[] {
        smoothingBlock1[0], smoothingBlock1[1],
        smoothingBlock2[0], smoothingBlock2[1]
    };

    for (int i = 0; i < 4; ++i)
    {
        if (! nearlyEqual (actualSmoothing[i], expectedSmoothing[i]))
        {
            std::cerr << "FAIL: smoothing의 " << i
                      << "번째 값이 예상값과 다릅니다.\n";
            return 1;
        }
    }

    if (smoother.isSmoothing())
    {
        std::cerr << "FAIL: 마지막 샘플 뒤에도 smoothing이 끝나지 않았습니다.\n";
        return 1;
    }

    std::cout << "PASS: Gain과 smoothing 테스트를 모두 통과했습니다.\n";
    return 0;
}
