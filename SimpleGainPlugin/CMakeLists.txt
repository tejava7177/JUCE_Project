cmake_minimum_required(VERSION 3.22)

if (APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

project(GainLab VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GAINLAB_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE" CACHE PATH "Path to JUCE")
set(GAINLAB_JUCE_VERSION "8.0.12" CACHE STRING "JUCE version used when no local checkout exists")

if (EXISTS "${GAINLAB_JUCE_PATH}/CMakeLists.txt")
    message(STATUS "GainLab: using local JUCE at ${GAINLAB_JUCE_PATH}")
    add_subdirectory("${GAINLAB_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
else()
    message(STATUS "GainLab: local JUCE not found; fetching ${GAINLAB_JUCE_VERSION}")
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${GAINLAB_JUCE_VERSION}
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(GainLab
    COMPANY_NAME               "Simjuheun"
    BUNDLE_ID                  "com.simjuheun.gainlab"
    PLUGIN_MANUFACTURER_CODE   Smjh
    PLUGIN_CODE                GnL1
    FORMATS                    AU VST3 Standalone
    PRODUCT_NAME               "GainLab"
    AU_MAIN_TYPE               kAudioUnitType_Effect
    VST3_CATEGORIES            Fx
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    COPY_PLUGIN_AFTER_BUILD    FALSE
)

target_sources(GainLab PRIVATE
    Source/DSP/DryWetDsp.h
    Source/DSP/GainDsp.h
    Source/DSP/LinearSmoother.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
)

target_compile_definitions(GainLab PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GainLab PRIVATE
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# DAW 없이도 Gain 계산을 확인할 수 있는 작은 실행 프로그램이다.
# JUCE나 플러그인 형식에 의존하지 않고 순수 DSP 코드만 테스트한다.
if (BUILD_TESTING)
    add_executable(GainLabDspTests
        Tests/GainDspTests.cpp
    )
    target_include_directories(GainLabDspTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(GainLabDspTests PRIVATE cxx_std_17)
    add_test(NAME GainLabDspTests COMMAND GainLabDspTests)
endif()
