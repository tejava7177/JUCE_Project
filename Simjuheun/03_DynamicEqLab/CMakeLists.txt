cmake_minimum_required(VERSION 3.22)

if (APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

project(DynamicEqLab VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DYNAMIC_EQ_LAB_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../JUCE" CACHE PATH "Path to JUCE")
set(DYNAMIC_EQ_LAB_JUCE_VERSION "8.0.12" CACHE STRING "JUCE version used when no local checkout exists")

if (EXISTS "${DYNAMIC_EQ_LAB_JUCE_PATH}/CMakeLists.txt")
    message(STATUS "DynamicEqLab: using local JUCE at ${DYNAMIC_EQ_LAB_JUCE_PATH}")
    add_subdirectory("${DYNAMIC_EQ_LAB_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
else()
    message(STATUS "DynamicEqLab: local JUCE not found; fetching ${DYNAMIC_EQ_LAB_JUCE_VERSION}")
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${DYNAMIC_EQ_LAB_JUCE_VERSION}
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(DynamicEqLab
    COMPANY_NAME               "Simjuheun"
    BUNDLE_ID                  "com.simjuheun.dynamiceqlab"
    PLUGIN_MANUFACTURER_CODE   Smjh
    PLUGIN_CODE                DyE1
    FORMATS                    AU VST3 Standalone
    PRODUCT_NAME               "DynamicEqLab"
    AU_MAIN_TYPE               kAudioUnitType_Effect
    VST3_CATEGORIES            Fx Filter
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    COPY_PLUGIN_AFTER_BUILD    FALSE
)

target_sources(DynamicEqLab PRIVATE
    Source/DSP/Biquad.h
    Source/DSP/DynamicBellDsp.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
)

target_compile_definitions(DynamicEqLab PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DynamicEqLab PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

if (BUILD_TESTING)
    add_executable(DynamicEqLabDspTests
        Tests/DynamicEqDspTests.cpp
    )
    target_include_directories(DynamicEqLabDspTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(DynamicEqLabDspTests PRIVATE cxx_std_17)
    add_test(NAME DynamicEqLabDspTests COMMAND DynamicEqLabDspTests)
endif()
