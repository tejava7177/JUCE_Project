#include "DSP/CompressorDsp.h"

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

std::vector<float> sine (double amplitude, double seconds, double frequency = 1000.0)
{
    const auto size = static_cast<int> (sampleRate * seconds);
    std::vector<float> result (static_cast<std::size_t> (size));
    for (int i = 0; i < size; ++i)
        result[static_cast<std::size_t> (i)] = static_cast<float> (
            amplitude * std::sin (2.0 * pi * frequency * i / sampleRate));
    return result;
}

double rmsLastQuarter (const std::vector<float>& samples)
{
    const auto begin = samples.size() * 3 / 4;
    double power = 0.0;
    for (auto i = begin; i < samples.size(); ++i)
        power += static_cast<double> (samples[i]) * samples[i];
    return std::sqrt (power / static_cast<double> (samples.size() - begin));
}

void processMono (compressor_lab::CompressorDsp& dsp, std::vector<float>& samples)
{
    auto* channel = samples.data();
    dsp.processBlock (&channel, 1, static_cast<int> (samples.size()));
}

void gainComputerTest()
{
    const auto reduction = compressor_lab::CompressorDsp::calculateReductionDb (
        -12.0, -24.0, 4.0);
    check (std::abs (reduction - 9.0) < 1.0e-9,
           "Threshold를 12 dB 넘은 4:1 신호는 9 dB 줄어야 한다");
    check (compressor_lab::CompressorDsp::calculateReductionDb (-30.0, -24.0, 4.0) == 0.0,
           "Threshold 아래에서는 Gain Reduction이 없어야 한다");
}

void belowThresholdPassesTest()
{
    compressor_lab::CompressorDsp dsp;
    dsp.prepare (sampleRate, 1);
    compressor_lab::CompressorParameters p;
    p.thresholdDb = -20.0;
    p.ratio = 4.0;
    p.attackMs = 5.0;
    p.releaseMs = 100.0;
    dsp.setParameters (p);

    auto signal = sine (0.01, 1.0); // Peak 약 -40 dBFS
    const auto before = rmsLastQuarter (signal);
    processMono (dsp, signal);
    const auto after = rmsLastQuarter (signal);

    check (dsp.reductionDb() < 0.01, "작은 신호는 압축되지 않아야 한다");
    check (std::abs (after / before - 1.0) < 0.01,
           "Makeup 0 dB에서 Threshold 아래 신호는 그대로여야 한다");
}

void loudSignalIsCompressedTest()
{
    compressor_lab::CompressorDsp dsp;
    dsp.prepare (sampleRate, 1);
    compressor_lab::CompressorParameters p;
    p.thresholdDb = -24.0;
    p.ratio = 4.0;
    p.attackMs = 2.0;
    p.releaseMs = 100.0;
    dsp.setParameters (p);

    auto signal = sine (0.5, 1.0); // Peak 약 -6 dBFS
    const auto before = rmsLastQuarter (signal);
    processMono (dsp, signal);
    const auto after = rmsLastQuarter (signal);
    const auto changeDb = 20.0 * std::log10 (after / before);

    check (dsp.reductionDb() > 12.0, "큰 신호에서는 충분한 Gain Reduction이 생겨야 한다");
    check (changeDb < -11.0, "실제 출력 레벨도 11 dB 이상 작아져야 한다");
}

void makeupGainTest()
{
    compressor_lab::CompressorDsp dsp;
    dsp.prepare (sampleRate, 1);
    compressor_lab::CompressorParameters p;
    p.thresholdDb = 0.0; // Compression은 동작하지 않음
    p.makeupDb = 6.0206;
    dsp.setParameters (p);

    auto signal = sine (0.05, 0.5);
    const auto before = rmsLastQuarter (signal);
    processMono (dsp, signal);
    const auto after = rmsLastQuarter (signal);
    check (std::abs (after / before - 2.0) < 0.01,
           "+6.0206 dB Makeup은 진폭을 약 2배로 만들어야 한다");
}

void stereoLinkTest()
{
    compressor_lab::CompressorDsp dsp;
    dsp.prepare (sampleRate, 2);
    compressor_lab::CompressorParameters p;
    p.thresholdDb = -24.0;
    p.ratio = 4.0;
    p.attackMs = 2.0;
    p.releaseMs = 100.0;
    dsp.setParameters (p);

    auto left = sine (0.5, 1.0);
    auto right = sine (0.02, 1.0);
    const auto leftBefore = rmsLastQuarter (left);
    const auto rightBefore = rmsLastQuarter (right);
    float* channels[] { left.data(), right.data() };
    dsp.processBlock (channels, 2, static_cast<int> (left.size()));
    const auto leftGain = rmsLastQuarter (left) / leftBefore;
    const auto rightGain = rmsLastQuarter (right) / rightBefore;

    check (std::abs (leftGain - rightGain) < 0.01,
           "스테레오 링크는 좌우에 같은 Gain Reduction을 적용해야 한다");
}
}

int main()
{
    std::cout << "CompressorLab DSP 테스트\n";
    std::cout << "입력 -> Envelope -> Gain Computer -> Makeup Gain -> 출력\n\n";

    gainComputerTest();
    belowThresholdPassesTest();
    loudSignalIsCompressedTest();
    makeupGainTest();
    stereoLinkTest();

    if (failures == 0)
    {
        std::cout << "PASS: 모든 Compressor DSP 테스트를 통과했습니다.\n";
        return 0;
    }

    std::cerr << failures << "개의 테스트가 실패했습니다.\n";
    return 1;
}
