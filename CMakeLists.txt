cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(ShaVeRMAHitMaker VERSION 1.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(HITMAKER_COPY_AFTER_BUILD "Copy plugins into the user's plug-in folders after build" OFF)
option(HITMAKER_BUILD_TESTS "Build the offline self-test / snapshot tool" OFF)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.16-security
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(HitMaker
    COMPANY_NAME                "Melody Makers"
    BUNDLE_ID                   com.melodymakers.shavermahitmaker
    PLUGIN_MANUFACTURER_CODE    MlMk
    PLUGIN_CODE                 ShHm
    FORMATS                     AU VST3 Standalone
    PRODUCT_NAME                "ShaVeRMA Hit Maker"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${HITMAKER_COPY_AFTER_BUILD}
    VST3_CATEGORIES             Fx Mastering
    AU_MAIN_TYPE                kAudioUnitType_Effect)

juce_add_binary_data(HitMakerAssets
    HEADER_NAME HitMakerAssets.h
    NAMESPACE   HitMakerAssets
    SOURCES
        assets/logo.png
        assets/fonts/Manrope-Regular.ttf
        assets/fonts/Manrope-SemiBold.ttf
        assets/fonts/Manrope-Bold.ttf
        assets/fonts/Manrope-ExtraBold.ttf
        assets/fonts/Unbounded-Bold.ttf
        assets/fonts/Unbounded-ExtraBold.ttf)

set(HITMAKER_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Jokes.cpp)

target_sources(HitMaker PRIVATE ${HITMAKER_SOURCES})

target_compile_definitions(HitMaker PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_compile_options(HitMaker PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(HitMaker
    PRIVATE
        HitMakerAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(HITMAKER_BUILD_TESTS)
    juce_add_console_app(HitMakerSelfTest PRODUCT_NAME "HitMakerSelfTest")
    target_sources(HitMakerSelfTest PRIVATE Tests/SelfTest.cpp ${HITMAKER_SOURCES})
    target_include_directories(HitMakerSelfTest PRIVATE Source)
    target_compile_definitions(HitMakerSelfTest PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        HITMAKER_SELFTEST=1)
    target_compile_options(HitMakerSelfTest PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    target_link_libraries(HitMakerSelfTest PRIVATE
        HitMakerAssets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
