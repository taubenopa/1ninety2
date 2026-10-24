cmake_minimum_required(VERSION 3.22)

# Statische MSVC-Runtime: Nutzer brauchen kein VC++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(NinetyBass VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(NinetyBassAssets SOURCES assets/logo.png)

juce_add_plugin(NinetyBass
    COMPANY_NAME "taubenopa"
    COMPANY_WEBSITE "https://github.com/taubenopa"
    PLUGIN_MANUFACTURER_CODE Taub
    PLUGIN_CODE Nbas
    FORMATS VST3
    PRODUCT_NAME "1ninety-bass"
    BUNDLE_ID "com.taubenopa.ninetybass"
    VST3_CATEGORIES Fx Distortion
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(NinetyBass)

target_sources(NinetyBass PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(NinetyBass PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(NinetyBass
    PRIVATE
        NinetyBassAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
