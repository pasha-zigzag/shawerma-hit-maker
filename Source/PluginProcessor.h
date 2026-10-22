#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

#include "DSP.h"

namespace ParamIDs
{
inline constexpr const char* meat    = "meat";
inline constexpr const char* sauce   = "sauce";
inline constexpr const char* spice   = "spice";
inline constexpr const char* lavash  = "lavash";
inline constexpr const char* ambient = "ambient";
inline constexpr const char* hit     = "hit";
inline constexpr const char* stas    = "stas";
inline constexpr const char* airhorn = "airhorn";
} // namespace ParamIDs

// Сколько режим «хит» добавляет к каждой крутилке (доля хода 0..1). Общие для DSP и UI.
namespace HitBoost
{
inline constexpr float meat   = 0.35f;
inline constexpr float sauce  = 0.35f;
inline constexpr float spice  = 0.30f;
inline constexpr float lavash = 0.25f;
inline constexpr float gainDb = 4.0f;
} // namespace HitBoost

class HitMakerProcessor : public juce::AudioProcessor
{
public:
    HitMakerProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ShaVeRMA Hit Maker"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Вызывается из UI: гудок прозвучит в следующем аудиоблоке.
    void triggerAirHorn() { hornRequested.store (true); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    void updateFilters (float meatAmount, float spiceAmount);

    std::atomic<float>* meatParam = nullptr;
    std::atomic<float>* sauceParam = nullptr;
    std::atomic<float>* spiceParam = nullptr;
    std::atomic<float>* lavashParam = nullptr;
    std::atomic<float>* ambientParam = nullptr;
    std::atomic<float>* hitParam = nullptr;
    std::atomic<float>* stasParam = nullptr;

    std::atomic<bool> hornRequested { false };

    double currentSampleRate = 44100.0;

    juce::SmoothedValue<float> meatSmoothed, spiceSmoothed, sauceSmoothed, widthSmoothed,
                               ambientSmoothed, gainSmoothed;

    std::array<hitmaker::Biquad, 2> lowShelf, highShelf;
    std::array<hitmaker::OnePole, 2> reverbHighpass;
    std::array<hitmaker::OctaveShifter, 2> shimmer;
    std::array<float, 2> lastWet {};

    juce::Reverb reverb;
    hitmaker::AirHorn horn;
    juce::dsp::Limiter<float> limiter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HitMakerProcessor)
};
