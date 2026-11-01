cmake_minimum_required(VERSION 3.22)
project(PEPITO_LA2A VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE is downloaded automatically when configuring the project.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(PepitoAssets
    SOURCES
        Resources/PepitoLA2A.png
)

juce_add_plugin(PEPITO_LA2A
    COMPANY_NAME "PEPITO Audio"
    BUNDLE_ID "com.pepitoaudio.la2a"
    PLUGIN_MANUFACTURER_CODE PpTo
    PLUGIN_CODE PpL2
    FORMATS VST3 Standalone
    PRODUCT_NAME "PEPITO LA2A"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

# Generate JuceHeader.h for the project source files.
juce_generate_juce_header(PEPITO_LA2A)

target_sources(PEPITO_LA2A PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(PEPITO_LA2A PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(PEPITO_LA2A PRIVATE
    PepitoAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
