#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

// Небольшие самописные DSP-кирпичики: без аллокаций в аудиопотоке.
namespace hitmaker
{
constexpr double kPi = 3.14159265358979323846;

// Биквад (RBJ cookbook), transposed direct form II.
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }

    void setShelf (double sampleRate, double freq, double gainDb, bool lowShelf)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * freq / sampleRate;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / 2.0 * std::sqrt (2.0); // S = 1
        const double k = 2.0 * std::sqrt (A) * alpha;

        double nb0, nb1, nb2, na0, na1, na2;

        if (lowShelf)
        {
            nb0 = A * ((A + 1) - (A - 1) * cw + k);
            nb1 = 2 * A * ((A - 1) - (A + 1) * cw);
            nb2 = A * ((A + 1) - (A - 1) * cw - k);
            na0 = (A + 1) + (A - 1) * cw + k;
            na1 = -2 * ((A - 1) + (A + 1) * cw);
            na2 = (A + 1) + (A - 1) * cw - k;
        }
        else
        {
            nb0 = A * ((A + 1) + (A - 1) * cw + k);
            nb1 = -2 * A * ((A - 1) + (A + 1) * cw);
            nb2 = A * ((A + 1) + (A - 1) * cw - k);
            na0 = (A + 1) - (A - 1) * cw + k;
            na1 = 2 * ((A - 1) - (A + 1) * cw);
            na2 = (A + 1) - (A - 1) * cw - k;
        }

        b0 = (float) (nb0 / na0);
        b1 = (float) (nb1 / na0);
        b2 = (float) (nb2 / na0);
        a1 = (float) (na1 / na0);
        a2 = (float) (na2 / na0);
    }

    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

// Однополюсный фильтр: lowpass, а highpass как x - lowpass.
struct OnePole
{
    float coeff = 0, state = 0;

    void setCutoff (double sampleRate, double freq) { coeff = (float) std::exp (-2.0 * kPi * freq / sampleRate); }
    void reset() { state = 0; }
    float lowpass (float x) { state = x + coeff * (state - x); return state; }
    float highpass (float x) { return x - lowpass (x); }
};

// Сдвиг на октаву вверх: две читающие головки по линии задержки с окном Ханна.
// Для шиммера этого более чем хватает.
struct OctaveShifter
{
    std::vector<float> buffer;
    int mask = 0, writePos = 0;
    float window = 2048.0f, phase = 0.0f;

    void prepare (double sampleRate)
    {
        window = (float) (sampleRate * 0.06);
        int size = 1;
        while (size < (int) window * 2 + 8)
            size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        reset();
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        phase = 0.0f;
    }

    float process (float x)
    {
        buffer[(size_t) writePos] = x;
        float out = 0.0f;

        for (int tap = 0; tap < 2; ++tap)
        {
            float p = phase + 0.5f * (float) tap;
            if (p >= 1.0f)
                p -= 1.0f;

            float readPos = (float) writePos - ((1.0f - p) * window + 1.0f);
            if (readPos < 0.0f)
                readPos += (float) (mask + 1);

            const int i0 = (int) readPos;
            const float frac = readPos - (float) i0;
            const float s0 = buffer[(size_t) (i0 & mask)];
            const float s1 = buffer[(size_t) ((i0 + 1) & mask)];
            const float g = (float) std::sin (kPi * p);
            out += (s0 + (s1 - s0) * frac) * g * g;
        }

        phase += 1.0f / window;
        if (phase >= 1.0f)
            phase -= 1.0f;

        writePos = (writePos + 1) & mask;
        return out;
    }
};

// Синтезированный гудок «БАА-БАА-БААААА». Никаких чужих сэмплов.
struct AirHorn
{
    double sampleRate = 44100.0;
    int position = -1, length = 0;
    double phases[6] {};
    OnePole tone;

    void prepare (double sr)
    {
        sampleRate = sr;
        length = (int) (sr * 1.75);
        position = -1;
        tone.setCutoff (sr, 3200.0);
        tone.reset();
    }

    void trigger() { position = 0; }
    bool isActive() const { return position >= 0; }

    float next()
    {
        if (position < 0)
            return 0.0f;

        const double t = position / sampleRate;
        static constexpr double starts[] = { 0.00, 0.40, 0.80 };
        static constexpr double ends[]   = { 0.30, 0.70, 1.70 };

        double env = 0.0, local = 0.0;
        for (int i = 0; i < 3; ++i)
        {
            if (t >= starts[i] && t < ends[i])
            {
                local = t - starts[i];
                const double attack = std::fmin (1.0, local / 0.012);
                const double release = std::fmin (1.0, (ends[i] - t) / 0.04);
                env = attack * release;
            }
        }

        // Небольшой «подъезд» по высоте в начале каждого гудка, как у настоящей дуделки.
        const double bend = 1.0 - 0.05 * std::exp (-local * 25.0);
        static constexpr double freqs[] = { 415.3, 417.1, 523.25, 521.4, 622.25, 624.6 };

        double sum = 0.0;
        for (int i = 0; i < 6; ++i)
        {
            phases[i] += freqs[i] * bend / sampleRate;
            if (phases[i] >= 1.0)
                phases[i] -= 1.0;
            sum += 2.0 * phases[i] - 1.0;
        }

        const float raw = (float) std::tanh (sum * 0.45) * (float) env;
        const float out = tone.lowpass (raw) * 0.32f;

        if (++position >= length)
            position = -1;

        return out;
    }
};
} // namespace hitmaker
