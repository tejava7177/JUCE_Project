#include "DSP/WideBandDeEsserDsp.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr double pi = 3.14159265358979323846;

void expect (bool condition, const std::string& message)
{
    if (! condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit (1);
    }
}

double rms (const std::vector<float>& samples, std::size_t start)
{
    double sum = 0.0;
    for (auto index = start; index < samples.size(); ++index)
        sum += static_cast<double> (samples[index]) * samples[index];
    return std::sqrt (sum / static_cast<double> (samples.size() - start));
}

std::vector<float> makeSine (double frequencyHz, double amplitude, int sampleCount)
{
    std::vector<float> result (static_cast<std::size_t> (sampleCount));
    for (int sample = 0; sample < sampleCount; ++sample)
        result[static_cast<std::size_t> (sample)] = static_cast<float> (
            amplitude * std::sin (2.0 * pi * frequencyHz * sample / sampleRate));
    return result;
}

deesser_lab::WideBandDeEsserParameters learningParameters()
{
    deesser_lab::WideBandDeEsserParameters parameters;
    parameters.detectorFrequencyHz = 7000.0;
    parameters.detectorQ = 4.0;
    parameters.thresholdDb = -24.0;
    parameters.ratio = 4.0;
    parameters.maxReductionDb = 10.0;
    parameters.attackMs = 0.2;
    parameters.releaseMs = 80.0;
    return parameters;
}
}

int main()
{
    using deesser_lab::WideBandDeEsserDsp;

    // 1. Threshold와 Ratio가 만드는 감쇄량 자체를 먼저 확인한다.
    expect (std::abs (WideBandDeEsserDsp::calculateReductionDb (
                         -12.0, -24.0, 4.0, 20.0) - 9.0) < 1.0e-9,
            "Threshold 위 12 dB, Ratio 4:1이면 Gain Reduction은 9 dB여야 합니다.");
    expect (std::abs (WideBandDeEsserDsp::calculateReductionDb (
                         -12.0, -24.0, 4.0, 6.0) - 6.0) < 1.0e-9,
            "Range는 최대 Gain Reduction을 제한해야 합니다.");

    // 2. 감지 중심인 7 kHz는 Threshold를 넘어서 충분히 감쇄되어야 한다.
    WideBandDeEsserDsp centerDetector;
    centerDetector.prepare (sampleRate, 1);
    centerDetector.setParameters (learningParameters());
    auto sibilance = makeSine (7000.0, 0.5, static_cast<int> (sampleRate));
    float* sibilanceChannels[] { sibilance.data() };
    centerDetector.processBlock (sibilanceChannels, 1, static_cast<int> (sibilance.size()));
    expect (centerDetector.reductionDb() > 8.0,
            "큰 7 kHz 신호는 치찰음으로 감지되어야 합니다.");

    // 3. 같은 크기여도 500 Hz는 Detector Band 밖이므로 거의 동작하지 않아야 한다.
    WideBandDeEsserDsp lowDetector;
    lowDetector.prepare (sampleRate, 1);
    lowDetector.setParameters (learningParameters());
    auto lowOnly = makeSine (500.0, 0.5, static_cast<int> (sampleRate));
    float* lowChannels[] { lowOnly.data() };
    lowDetector.processBlock (lowChannels, 1, static_cast<int> (lowOnly.size()));
    expect (lowDetector.reductionDb() < 0.5,
            "저역만 있을 때는 치찰음 감지기가 거의 동작하지 않아야 합니다.");

    // 4. Wide-band 확인: 왼쪽의 7 kHz가 감지되면 오른쪽의 500 Hz도 함께 줄어든다.
    //    즉, '고역만 감지하지만 줄이는 대상은 원본 전체'라는 뜻이다.
    WideBandDeEsserDsp stereoLinked;
    stereoLinked.prepare (sampleRate, 2);
    stereoLinked.setParameters (learningParameters());
    auto leftSibilance = makeSine (7000.0, 0.5, static_cast<int> (sampleRate));
    auto rightLow = makeSine (500.0, 0.25, static_cast<int> (sampleRate));
    const auto referenceLowRms = rms (rightLow, rightLow.size() / 2);
    float* stereoChannels[] { leftSibilance.data(), rightLow.data() };
    stereoLinked.processBlock (stereoChannels, 2, static_cast<int> (rightLow.size()));
    const auto processedLowRms = rms (rightLow, rightLow.size() / 2);
    const auto lowReductionDb = -20.0 * std::log10 (processedLowRms / referenceLowRms);
    expect (lowReductionDb > 8.0,
            "치찰음이 검출되면 반대 채널의 저역까지 함께 줄어야 Wide-band입니다.");

    // 5. Detector Listen에서는 원본이 아니라 Band-pass로 추출한 대역만 들려야 한다.
    auto listenParameters = learningParameters();
    listenParameters.detectorListen = true;
    WideBandDeEsserDsp listenCenter;
    listenCenter.prepare (sampleRate, 1);
    listenCenter.setParameters (listenParameters);
    auto listenHigh = makeSine (7000.0, 0.5, static_cast<int> (sampleRate / 2.0));
    float* listenHighChannels[] { listenHigh.data() };
    listenCenter.processBlock (listenHighChannels, 1, static_cast<int> (listenHigh.size()));

    WideBandDeEsserDsp listenLow;
    listenLow.prepare (sampleRate, 1);
    listenLow.setParameters (listenParameters);
    auto listenBass = makeSine (500.0, 0.5, static_cast<int> (sampleRate / 2.0));
    float* listenLowChannels[] { listenBass.data() };
    listenLow.processBlock (listenLowChannels, 1, static_cast<int> (listenBass.size()));
    expect (rms (listenHigh, listenHigh.size() / 2)
                > rms (listenBass, listenBass.size() / 2) * 10.0,
            "Detector Listen은 7 kHz를 500 Hz보다 훨씬 크게 들려줘야 합니다.");

    // 6. 입력이 사라지면 Release 시간에 따라 감쇄가 다시 0 dB로 돌아온다.
    auto silence = std::vector<float> (static_cast<std::size_t> (sampleRate), 0.0f);
    float* silenceChannels[] { silence.data() };
    centerDetector.processBlock (silenceChannels, 1, static_cast<int> (silence.size()));
    expect (centerDetector.reductionDb() < 0.1,
            "치찰음이 끝난 뒤에는 Release를 거쳐 Gain Reduction이 풀려야 합니다.");

    std::cout << "PASS: Wide-band De-esser DSP 테스트를 모두 통과했습니다.\n"
              << "  - 7 kHz 감지 / 500 Hz 비감지\n"
              << "  - Stereo-linked 전체 대역 감쇄\n"
              << "  - Detector Listen / Release 복귀\n";
    return 0;
}
