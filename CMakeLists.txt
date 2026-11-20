cmake_minimum_required(VERSION 3.22)
project(MIDIstudio VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
endif()
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)
set(MIDISTUDIO_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND MIDISTUDIO_FORMATS AU)
endif()
juce_add_plugin(MIDIstudio
    COMPANY_NAME "solaraudio"
    COMPANY_WEBSITE "https://github.com/solarUpdateCode/MIDIstudio"
    BUNDLE_ID "com.solaraudio.midistudio"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE SlAu
    PLUGIN_CODE MdSt
    FORMATS ${MIDISTUDIO_FORMATS}
    PRODUCT_NAME "MIDIstudio"
)
target_sources(MIDIstudio PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)
target_compile_definitions(MIDIstudio PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)
target_link_libraries(MIDIstudio
    PRIVATE juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags
)
