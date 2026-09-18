cmake_minimum_required(VERSION 3.22)

if (APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

project(EqLab VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EQLAB_JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../JUCE" CACHE PATH "Path to JUCE")
set(EQLAB_JUCE_VERSION "8.0.12" CACHE STRING "JUCE version used when no local checkout exists")

if (EXISTS "${EQLAB_JUCE_PATH}/CMakeLists.txt")
    message(STATUS "EqLab: using local JUCE at ${EQLAB_JUCE_PATH}")
    add_subdirectory("${EQLAB_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
else()
    message(STATUS "EqLab: local JUCE not found; fetching ${EQLAB_JUCE_VERSION}")
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${EQLAB_JUCE_VERSION}
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(EqLab
    COMPANY_NAME               "Simjuheun"
    BUNDLE_ID                  "com.simjuheun.eqlab"
    PLUGIN_MANUFACTURER_CODE   Smjh
    PLUGIN_CODE                EqL1
    FORMATS                    AU VST3 Standalone
    PRODUCT_NAME               "EqLab"
    AU_MAIN_TYPE               kAudioUnitType_Effect
    VST3_CATEGORIES            Fx Filter
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    COPY_PLUGIN_AFTER_BUILD    FALSE
)

target_sources(EqLab PRIVATE
    Source/DSP/BiquadBellDsp.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
)

target_compile_definitions(EqLab PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(EqLab PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

if (BUILD_TESTING)
    add_executable(EqLabDspTests
        Tests/EqLabDspTests.cpp
    )
    target_include_directories(EqLabDspTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_features(EqLabDspTests PRIVATE cxx_std_17)
    add_test(NAME EqLabDspTests COMMAND EqLabDspTests)
endif()
