#include "DSP/DynamicBellDsp.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr double sampleRate = 48000.0;

int failures = 0;

void check (bool condition, const std::string& message)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::vector<float> sine (double frequencyHz, double amplitude, double seconds)
{
    const auto count = static_cast<int> (sampleRate * seconds);
    std::vector<float> samples (static_cast<std::size_t> (count));
    for (int i = 0; i < count; ++i)
        samples[static_cast<std::size_t> (i)] = static_cast<float> (
            amplitude * std::sin (2.0 * pi * frequencyHz * i / sampleRate));
    return samples;
}

double rmsOfLastQuarter (const std::vector<float>& samples)
{
    const auto begin = samples.size() * 3 / 4;
    double sum = 0.0;
    for (auto i = begin; i < samples.size(); ++i)
        sum += static_cast<double> (samples[i]) * samples[i];
    return std::sqrt (sum / static_cast<double> (samples.size() - begin));
}

void processMono (dynamic_eq_lab::DynamicBellDsp& dsp, std::vector<float>& samples)
{
    auto* channel = samples.data();
    dsp.processBlock (&channel, 1, static_cast<int> (samples.size()));
}

void ratioMathTest()
{
    const auto reduction = dynamic_eq_lab::DynamicBellDsp::calculateTargetReductionDb (
        -12.0, -24.0, 4.0, 18.0);
    check (std::abs (reduction - 9.0) < 1.0e-9,
           "12 dB 초과, 4:1 Ratio는 9 dB 감쇄여야 한다");

    const auto limited = dynamic_eq_lab::DynamicBellDsp::calculateTargetReductionDb (
        -12.0, -24.0, 4.0, 6.0);
    check (std::abs (limited - 6.0) < 1.0e-9,
           "Range 6 dB가 계산된 9 dB 감쇄를 제한해야 한다");

    const auto below = dynamic_eq_lab::DynamicBellDsp::calculateTargetReductionDb (
        -30.0, -24.0, 4.0, 6.0);
    check (below == 0.0, "Threshold 아래에서는 감쇄가 없어야 한다");
}

void belowThresholdTest()
{
    dynamic_eq_lab::DynamicBellDsp dsp;
    dsp.prepare (sampleRate, 1);
    dynamic_eq_lab::DynamicBellParameters p;
    p.frequencyHz = 1000.0;
    p.q = 4.0;
    p.thresholdDb = -20.0;
    p.ratio = 4.0;
    p.maxReductionDb = 12.0;
    p.attackMs = 5.0;
    p.releaseMs = 80.0;
    dsp.setParameters (p);

    auto input = sine (1000.0, 0.01, 1.0); // peak 약 -40 dBFS
    const auto inputRms = rmsOfLastQuarter (input);
    processMono (dsp, input);
    const auto outputRms = rmsOfLastQuarter (input);

    check (dsp.reductionDb() < 0.01,
           "선택 대역이 Threshold 아래이면 Gain Reduction이 0이어야 한다");
    check (std::abs (outputRms / inputRms - 1.0) < 0.01,
           "Threshold 아래의 신호는 거의 그대로 통과해야 한다");
}

void loudCentreFrequencyTest()
{
    dynamic_eq_lab::DynamicBellDsp dsp;
    dsp.prepare (sampleRate, 1);
    dynamic_eq_lab::DynamicBellParameters p;
    p.frequencyHz = 1000.0;
    p.q = 4.0;
    p.thresholdDb = -24.0;
    p.ratio = 4.0;
    p.maxReductionDb = 6.0;
    p.attackMs = 5.0;
    p.releaseMs = 100.0;
    dsp.setParameters (p);

    auto input = sine (1000.0, 0.5, 1.0);
    const auto inputRms = rmsOfLastQuarter (input);
    processMono (dsp, input);
    const auto outputRms = rmsOfLastQuarter (input);
    const auto outputGainDb = 20.0 * std::log10 (outputRms / inputRms);

    check (dsp.reductionDb() > 5.8,
           "큰 중심 주파수는 Range 6 dB 근처까지 감쇄되어야 한다");
    check (outputGainDb < -5.5 && outputGainDb > -6.5,
           "중심 주파수의 실제 출력도 약 6 dB 작아져야 한다");
}

void detectorIsFrequencySelectiveTest()
{
    dynamic_eq_lab::DynamicBellDsp dsp;
    dsp.prepare (sampleRate, 1);
    dynamic_eq_lab::DynamicBellParameters p;
    p.frequencyHz = 1000.0;
    p.q = 4.0;
    p.thresholdDb = -24.0;
    p.ratio = 4.0;
    p.maxReductionDb = 6.0;
    p.attackMs = 5.0;
    p.releaseMs = 80.0;
    dsp.setParameters (p);

    auto input = sine (100.0, 0.5, 1.0);
    processMono (dsp, input);

    check (dsp.reductionDb() < 0.1,
           "1 kHz를 감지할 때 큰 100 Hz 신호만으로는 동작하지 않아야 한다");
}

void releaseTest()
{
    dynamic_eq_lab::DynamicBellDsp dsp;
    dsp.prepare (sampleRate, 1);
    dynamic_eq_lab::DynamicBellParameters p;
    p.frequencyHz = 1000.0;
    p.q = 4.0;
    p.thresholdDb = -24.0;
    p.ratio = 4.0;
    p.maxReductionDb = 6.0;
    p.attackMs = 2.0;
    p.releaseMs = 100.0;
    dsp.setParameters (p);

    auto loud = sine (1000.0, 0.5, 0.5);
    processMono (dsp, loud);
    check (dsp.reductionDb() > 5.0, "큰 신호에서 먼저 감쇄가 발생해야 한다");

    std::vector<float> silence (static_cast<std::size_t> (sampleRate), 0.0f);
    processMono (dsp, silence);
    check (dsp.reductionDb() < 0.05,
           "신호가 사라지면 Release를 거쳐 Gain Reduction이 0으로 돌아와야 한다");
}
}

int main()
{
    std::cout << "DynamicEqLab DSP 테스트\n";
    std::cout << "감지 경로: Band-pass -> Envelope -> Threshold/Ratio\n";
    std::cout << "처리 경로: 계산된 감쇄량 -> Bell EQ Gain\n\n";

    ratioMathTest();
    belowThresholdTest();
    loudCentreFrequencyTest();
    detectorIsFrequencySelectiveTest();
    releaseTest();

    if (failures == 0)
    {
        std::cout << "PASS: 모든 Dynamic EQ DSP 테스트를 통과했습니다.\n";
        return 0;
    }

    std::cerr << failures << "개의 테스트가 실패했습니다.\n";
    return 1;
}
