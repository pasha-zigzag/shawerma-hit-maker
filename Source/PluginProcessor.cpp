#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::String utf8 (const char* text) { return juce::String::fromUTF8 (text); }

std::unique_ptr<juce::AudioParameterFloat> makeKnob (const char* id, const char* name, float defaultValue)
{
    auto attributes = juce::AudioParameterFloatAttributes()
                          .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
                          .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue() / 100.0f; });

    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, utf8 (name),
                                                        juce::NormalisableRange<float> (0.0f, 1.0f), defaultValue,
                                                        attributes);
}

float clamp01 (float v) { return juce::jlimit (0.0f, 1.0f, v); }
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout HitMakerProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (makeKnob (ParamIDs::meat,    "Мясо (низ)",       0.0f));
    layout.add (makeKnob (ParamIDs::sauce,   "Соус (тепло)",     0.0f));
    layout.add (makeKnob (ParamIDs::spice,   "Острота (верх)",   0.0f));
    layout.add (makeKnob (ParamIDs::lavash,  "Лаваш (ширина)",   0.0f));
    layout.add (makeKnob (ParamIDs::ambient, "Эмбиент (Марина)", 0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::hit, 1 }, utf8 ("Хит"), false));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::stas, 1 }, utf8 ("Режим Стаса +6 dB"), false));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::airhorn, 1 }, "AIR HORN", false));

    return layout;
}

HitMakerProcessor::HitMakerProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ShaVeRMA", createParameterLayout())
{
    meatParam    = apvts.getRawParameterValue (ParamIDs::meat);
    sauceParam   = apvts.getRawParameterValue (ParamIDs::sauce);
    spiceParam   = apvts.getRawParameterValue (ParamIDs::spice);
    lavashParam  = apvts.getRawParameterValue (ParamIDs::lavash);
    ambientParam = apvts.getRawParameterValue (ParamIDs::ambient);
    hitParam     = apvts.getRawParameterValue (ParamIDs::hit);
    stasParam    = apvts.getRawParameterValue (ParamIDs::stas);
}

bool HitMakerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void HitMakerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    for (auto* smoothed : { &meatSmoothed, &spiceSmoothed, &sauceSmoothed, &widthSmoothed, &ambientSmoothed })
        smoothed->reset (sampleRate, 0.08);

    // Включение хита — плавный «въезд» громкости, чтобы не было щелчка.
    gainSmoothed.reset (sampleRate, 0.25);
    gainSmoothed.setCurrentAndTargetValue (1.0f);

    for (int ch = 0; ch < 2; ++ch)
    {
        lowShelf[(size_t) ch].reset();
        highShelf[(size_t) ch].reset();
        reverbHighpass[(size_t) ch].setCutoff (sampleRate, 220.0);
        reverbHighpass[(size_t) ch].reset();
        shimmer[(size_t) ch].prepare (sampleRate);
        lastWet[(size_t) ch] = 0.0f;
    }

    reverb.setSampleRate (sampleRate);
    reverb.reset();

    horn.prepare (sampleRate);

    limiter.prepare ({ sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 });
    limiter.setThreshold (-0.8f);
    limiter.setRelease (120.0f);
    limiter.reset();

    updateFilters (0.0f, 0.0f);
}

void HitMakerProcessor::updateFilters (float meatAmount, float spiceAmount)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        lowShelf[(size_t) ch].setShelf (currentSampleRate, 110.0, meatAmount * 9.0, true);
        highShelf[(size_t) ch].setShelf (currentSampleRate, 7500.0, spiceAmount * 8.0, false);
    }
}

void HitMakerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (2, getTotalNumOutputChannels());

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (hornRequested.exchange (false))
        horn.trigger();

    // Режим «хит» не отдельная обработка: он подкручивает все ингредиенты сразу.
    const float hit = hitParam->load() > 0.5f ? 1.0f : 0.0f;
    const float stas = stasParam->load() > 0.5f ? 1.0f : 0.0f;

    meatSmoothed.setTargetValue (clamp01 (meatParam->load() + hit * 0.35f));
    spiceSmoothed.setTargetValue (clamp01 (spiceParam->load() + hit * 0.30f));
    sauceSmoothed.setTargetValue (clamp01 (sauceParam->load() + hit * 0.35f));
    widthSmoothed.setTargetValue (1.0f + clamp01 (lavashParam->load() + hit * 0.25f));
    ambientSmoothed.setTargetValue (ambientParam->load());
    gainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (hit * 4.0f + stas * 6.0f));

    // Фильтры пересчитываем раз в блок — этого достаточно и без аллокаций.
    meatSmoothed.skip (numSamples);
    spiceSmoothed.skip (numSamples);
    updateFilters (meatSmoothed.getCurrentValue(), spiceSmoothed.getCurrentValue());

    const float amb = ambientSmoothed.getTargetValue();
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.55f + 0.33f * amb;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 0.33f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters (reverbParams);

    auto* left = buffer.getWritePointer (0);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float sauce = sauceSmoothed.getNextValue();
        const float width = widthSmoothed.getNextValue();
        const float ambient = ambientSmoothed.getNextValue();
        const float gain = gainSmoothed.getNextValue();

        float samples[2] = { left[i], right != nullptr ? right[i] : left[i] };

        // EQ + сатурация: tanh с компенсацией, на малых уровнях даёт плотность, на пиках мягко скругляет.
        const float drive = 1.0f + sauce * 5.0f;
        const float makeup = 1.0f / std::sqrt (drive);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float x = lowShelf[(size_t) ch].process (samples[ch]);
            x = highShelf[(size_t) ch].process (x);
            const float saturated = std::tanh (drive * x) * makeup;
            samples[ch] = x + sauce * (saturated - x);
        }

        if (numChannels > 1)
        {
            const float mid = 0.5f * (samples[0] + samples[1]);
            const float side = 0.5f * (samples[0] - samples[1]) * width;
            samples[0] = mid + side;
            samples[1] = mid - side;
        }

        // Ревер с шиммером: часть хвоста уходит на октаву вверх и обратно в ревер.
        const float feedback = 0.12f * ambient;
        float wet[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            const float source = samples[juce::jmin (ch, numChannels - 1)];
            const float shimmerIn = std::tanh (shimmer[(size_t) ch].process (lastWet[(size_t) ch]));
            wet[ch] = reverbHighpass[(size_t) ch].highpass (source) + feedback * shimmerIn;
        }

        reverb.processStereo (&wet[0], &wet[1], 1);
        lastWet[0] = wet[0];
        lastWet[1] = wet[1];

        const float hornSample = horn.next();
        const float dryLevel = 1.0f - 0.35f * ambient;
        const float wetLevel = 0.8f * ambient;

        for (int ch = 0; ch < numChannels; ++ch)
            samples[ch] = (samples[ch] * dryLevel + wet[ch] * wetLevel + hornSample) * gain;

        left[i] = samples[0];
        if (right != nullptr)
            right[i] = samples[1];
    }

    // Лимитер всегда на выходе: как бы ни крутили — без клиппинга.
    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numChannels, (size_t) numSamples);
    limiter.process (juce::dsp::ProcessContextReplacing<float> (block));

    for (int ch = 0; ch < numChannels; ++ch)
        juce::FloatVectorOperations::clip (buffer.getWritePointer (ch), buffer.getReadPointer (ch), -0.999f, 0.999f, numSamples);
}

juce::AudioProcessorEditor* HitMakerProcessor::createEditor()
{
    return new HitMakerEditor (*this);
}

void HitMakerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HitMakerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

#if ! HITMAKER_SELFTEST
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HitMakerProcessor();
}
#endif
