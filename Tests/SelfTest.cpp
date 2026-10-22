// Оффлайн-проверка: гоняем звук через процессор и снимаем скриншоты интерфейса.
// Запуск: HitMakerSelfTest <папка-для-png>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlock = 512;

void setParam (HitMakerProcessor& p, const char* id, float value)
{
    p.apvts.getParameter (id)->setValueNotifyingHost (value);
}

struct Stats
{
    float peak = 0.0f;
    double rms = 0.0;
    bool finite = true;
    float tailPeak = 0.0f;
};

// Тестовый сигнал: шум + синус 110 Гц, -9 dBFS пик, затем тишина (проверяем хвост ревера).
Stats run (HitMakerProcessor& p, double seconds, double silentTail, bool triggerHorn = false)
{
    p.prepareToPlay (kSampleRate, kBlock);
    juce::Random rng (42);
    juce::AudioBuffer<float> buffer (2, kBlock);
    juce::MidiBuffer midi;
    Stats stats;
    double sumSquares = 0.0;
    long count = 0;
    double phase = 0.0;
    float tailPeak = 0.0f;

    if (triggerHorn)
        p.triggerAirHorn();

    const int totalBlocks = (int) ((seconds + silentTail) * kSampleRate / kBlock);
    const int loudBlocks = (int) (seconds * kSampleRate / kBlock);

    for (int b = 0; b < totalBlocks; ++b)
    {
        for (int i = 0; i < kBlock; ++i)
        {
            float s = 0.0f;
            if (b < loudBlocks)
            {
                phase += 110.0 / kSampleRate;
                s = 0.2f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * phase) + 0.15f * (rng.nextFloat() * 2.0f - 1.0f);
            }
            buffer.setSample (0, i, s);
            buffer.setSample (1, i, s * 0.8f + 0.05f * (rng.nextFloat() * 2.0f - 1.0f) * (b < loudBlocks ? 1.0f : 0.0f));
        }

        p.processBlock (buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < kBlock; ++i)
            {
                const float v = buffer.getSample (ch, i);
                if (! std::isfinite (v))
                    stats.finite = false;
                stats.peak = juce::jmax (stats.peak, std::abs (v));
                if (b < loudBlocks)
                {
                    sumSquares += (double) v * v;
                    ++count;
                }
                if (b >= totalBlocks - 10)
                    tailPeak = juce::jmax (tailPeak, std::abs (v));
            }
    }

    stats.tailPeak = tailPeak;
    stats.rms = std::sqrt (sumSquares / (double) juce::jmax (1L, count));
    juce::Logger::writeToLog ("    tail peak after silence: " + juce::String (juce::Decibels::gainToDecibels (tailPeak), 1) + " dB");
    return stats;
}

bool report (const juce::String& name, const Stats& s)
{
    const bool ok = s.finite && s.peak <= 1.0f;
    juce::Logger::writeToLog ((ok ? "[OK]   " : "[FAIL] ") + name
                              + "  peak " + juce::String (juce::Decibels::gainToDecibels (s.peak), 2) + " dB"
                              + "  rms " + juce::String (juce::Decibels::gainToDecibels ((float) s.rms), 2) + " dB"
                              + (s.finite ? "" : "  NaN/Inf!"));
    return ok;
}

void snapshot (juce::Component& c, const juce::File& file)
{
    auto image = c.createComponentSnapshot (c.getLocalBounds(), true, 2.0f);
    file.deleteFile();
    juce::FileOutputStream out (file);
    juce::PNGImageFormat().writeImageToStream (image, out);
    juce::Logger::writeToLog ("snapshot: " + file.getFullPathName());
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    bool allOk = true;

    {
        HitMakerProcessor p;
        allOk &= report ("default", run (p, 3.0, 1.0));
    }
    {
        HitMakerProcessor p;
        for (auto* id : { ParamIDs::meat, ParamIDs::sauce, ParamIDs::spice, ParamIDs::lavash })
            setParam (p, id, 1.0f);
        allOk &= report ("all knobs max", run (p, 3.0, 1.0));
    }
    {
        HitMakerProcessor p;
        setParam (p, ParamIDs::hit, 1.0f);
        allOk &= report ("hit", run (p, 3.0, 1.0));
    }
    {
        HitMakerProcessor p;
        for (auto* id : { ParamIDs::meat, ParamIDs::sauce, ParamIDs::spice, ParamIDs::lavash, ParamIDs::ambient, ParamIDs::hit, ParamIDs::stas })
            setParam (p, id, 1.0f);
        allOk &= report ("everything max + hit + stas", run (p, 6.0, 1.0, true));
    }
    {
        // Шиммер с обратной связью не должен разгоняться: после 20 с тишины хвост обязан затухнуть.
        HitMakerProcessor p;
        setParam (p, ParamIDs::ambient, 1.0f);
        const auto s = run (p, 5.0, 20.0);
        allOk &= report ("ambient max, long tail", s);
        const bool decays = s.tailPeak < juce::Decibels::decibelsToGain (-60.0f);
        juce::Logger::writeToLog (decays ? "[OK]   shimmer tail decays" : "[FAIL] shimmer tail does not decay");
        allOk &= decays;
    }
    {
        juce::Logger::writeToLog ("air horn on silence:");
        HitMakerProcessor q;
        q.prepareToPlay (kSampleRate, kBlock);
        juce::AudioBuffer<float> buffer (2, kBlock);
        juce::MidiBuffer midi;
        q.triggerAirHorn();
        float peak = 0.0f;
        for (int b = 0; b < 200; ++b)
        {
            buffer.clear();
            q.processBlock (buffer, midi);
            peak = juce::jmax (peak, buffer.getMagnitude (0, kBlock));
        }
        juce::Logger::writeToLog ("    horn peak " + juce::String (juce::Decibels::gainToDecibels (peak), 1) + " dB");
        allOk &= peak > 0.05f && peak <= 1.0f;
    }

    if (argc > 1)
    {
        const juce::File dir (argv[1]);
        dir.createDirectory();

        HitMakerProcessor p;
        p.prepareToPlay (kSampleRate, kBlock);
        setParam (p, ParamIDs::meat, 0.6f);
        setParam (p, ParamIDs::sauce, 0.35f);
        setParam (p, ParamIDs::ambient, 0.2f);

        std::unique_ptr<juce::AudioProcessorEditor> base (p.createEditorAndMakeActive());
        auto& editor = dynamic_cast<HitMakerEditor&> (*base);

        editor.debugAdvance (0.1);
        snapshot (editor, dir.getChildFile ("1-idle.png"));

        editor.debugStartCooking();
        editor.debugAdvance (2.5);
        snapshot (editor, dir.getChildFile ("2-cooking.png"));

        setParam (p, ParamIDs::stas, 1.0f);
        setParam (p, ParamIDs::airhorn, 1.0f);
        editor.debugSetIdea (juce::String::fromUTF8 ("Хаус на баяне в 124 BPM. Главный звук — скрип лаваша (записывать аккуратно). Фишка: дроп — это 4 секунды тишины."));
        editor.debugAdvance (5.5);
        snapshot (editor, dir.getChildFile ("3-done.png"));

        editor.debugShowCredits();
        snapshot (editor, dir.getChildFile ("4-credits.png"));

    }

    juce::Logger::writeToLog (allOk ? "ALL OK" : "SOME CHECKS FAILED");
    return allOk ? 0 : 1;
}
