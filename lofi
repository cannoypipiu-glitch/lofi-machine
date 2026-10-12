cmake_minimum_required(VERSION 3.22)

# Runtime estático: o plugin roda sem precisar instalar nada do Visual C++
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(LoFiMachine VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(LoFiMachine
    COMPANY_NAME "LoFiLab"
    PLUGIN_MANUFACTURER_CODE Lofi
    PLUGIN_CODE Lfm1
    FORMATS VST3
    PRODUCT_NAME "LoFi Machine"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(LoFiMachine PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h)

target_compile_definitions(LoFiMachine PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(LoFiMachine
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags)
