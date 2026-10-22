#pragma once
#include "Parameters.h"
#include "OversamplingFilter.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace sundrive {
constexpr double pi = 3.14159265358979323846;

// Transposed direct-form II. Coefficients change only at a bounded control rate;
// the normalized controls themselves are smoothed at audio rate.
struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    double tick(double x) {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        if (std::abs(z1) < 1.e-25)
            z1 = 0;
        if (std::abs(z2) < 1.e-25)
            z2 = 0;
        return y;
    }
    void set(double B0, double B1, double B2, double A0, double A1, double A2) {
        b0 = B0 / A0;
        b1 = B1 / A0;
        b2 = B2 / A0;
        a1 = A1 / A0;
        a2 = A2 / A0;
    }
    void lowpass(double sr, double hz) {
        const double w = 2 * pi * std::min(hz, sr * .44) / sr, c = std::cos(w),
                     a = std::sin(w) / std::sqrt(2.0);
        set((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + a, -2 * c, 1 - a);
    }
    void highpass(double sr, double hz) {
        const double w = 2 * pi * hz / sr, c = std::cos(w), a = std::sin(w) / std::sqrt(2.0);
        set((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + a, -2 * c, 1 - a);
    }
    void peak(double sr, double hz, double q, double db) {
        const double A = std::pow(10.0, db / 40), w = 2 * pi * hz / sr, c = std::cos(w),
                     a = std::sin(w) / (2 * q);
        set(1 + a * A, -2 * c, 1 - a * A, 1 + a / A, -2 * c, 1 - a / A);
    }
    void shelf(double sr, double hz, double db, bool high) {
        const double A = std::pow(10.0, db / 40), w = 2 * pi * std::min(hz, sr * .4) / sr,
                     c = std::cos(w);
        const double r = std::sin(w) * std::sqrt(2 * A);
        if (!high)
            set(A * ((A + 1) - (A - 1) * c + r), 2 * A * ((A - 1) - (A + 1) * c),
                A * ((A + 1) - (A - 1) * c - r), (A + 1) + (A - 1) * c + r,
                -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - r);
        else
            set(A * ((A + 1) + (A - 1) * c + r), -2 * A * ((A - 1) + (A + 1) * c),
                A * ((A + 1) + (A - 1) * c - r), (A + 1) - (A - 1) * c + r,
                2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - r);
    }
};

// Minimum-phase versions of the original 129-tap anti-alias FIRs retain their
// magnitude response while reducing the round-trip delay from 32 to ~4.824 samples.
// A first-order fractional allpass aligns the DC delay to 5 whole base samples.
// All histories and filter coefficients are allocated before process().
struct Oversampler {
    static constexpr int taps = oversampling_filter::taps, history = (taps + 3) / 4,
                         latency = oversampling_filter::latency;
    inline static constexpr auto h = oversampling_filter::coefficients;
    std::array<double, history * 2> input{};
    std::array<double, taps * 2> output{};
    int ip = 0, op = 0;
    double alignInput = 0, alignOutput = 0;
    void reset() {
        input.fill(0);
        output.fill(0);
        ip = op = 0;
        alignInput = alignOutput = 0;
    }
    template <class Shape> double tick(double x, const Shape &shape) {
        input[ip] = input[ip + history] = x;
        const double *newest = &input[ip + history];
        double result = 0;
        for (int phase = 0; phase < 4; ++phase) {
            double up = 0;
            for (int k = phase, j = 0; k < taps; k += 4, ++j)
                up += h[k] * newest[-j];
            const double y = shape(up * 4);
            output[op] = output[op + taps] = y;
            if (phase == 0) {
                const double *n = &output[op + taps];
                for (int k = 0; k < taps; ++k)
                    result += h[k] * n[-k];
            }
            if (++op == taps)
                op = 0;
        }
        if (++ip == history)
            ip = 0;
        constexpr double a = oversampling_filter::alignmentCoefficient;
        const double aligned = a * result + alignInput - a * alignOutput;
        alignInput = result;
        alignOutput = std::abs(aligned) < 1.e-25 ? 0 : aligned;
        return aligned;
    }
};

struct Channel {
    Oversampler oversampler;
    Biquad inputCut, preMid, tone, bass, mid, treble;
    std::array<double, Oversampler::latency + 1> dry{};
    int dryPos = 0;
    double dcX = 0, dcY = 0;
    void reset() {
        oversampler.reset();
        inputCut.reset();
        preMid.reset();
        tone.reset();
        bass.reset();
        mid.reset();
        treble.reset();
        dry.fill(0);
        dryPos = 0;
        dcX = dcY = 0;
    }
    double delayed(double x) {
        dry[dryPos] = x;
        const double out = dry[(dryPos + 1) % dry.size()];
        dryPos = (dryPos + 1) % dry.size();
        return out;
    }
};

class Engine {
  public:
    static constexpr int latency = Oversampler::latency;
    void prepare(double sampleRate, const Values &initial = defaults()) {
        sr = sampleRate;
        smoothing = 1 - std::exp(-1 / (.020 * sr));
        dcR = std::exp(-2 * pi * 8 / sr);
        target = smoothed = initial;
        clipWeights = {0, 0, 0};
        clipWeights[static_cast<int>(std::round(initial[Clipping] * 2))] = 1;
        for (auto &c : channels)
            c.reset();
        counter = 0;
        updateFilters();
    }
    void set(unsigned id, double value) {
        if (id < Count)
            target[id] = canonical(id, value);
    }
    void setAll(const Values &v) {
        for (unsigned i = 0; i < Count; ++i)
            set(i, v[i]);
    }
    const Values &values() const { return target; }
    void beginSample() {
        for (unsigned i = 0; i < Count; ++i)
            smoothed[i] += (target[i] - smoothed[i]) * smoothing;
        const int chosen = static_cast<int>(std::round(target[Clipping] * 2));
        for (int i = 0; i < 3; ++i)
            clipWeights[i] += ((chosen == i ? 1.0 : 0.0) - clipWeights[i]) * smoothing;
        if ((counter++ & 15) == 0)
            updateFilters();
        drive = 1 + 49 * smoothed[Drive] * smoothed[Drive];
        compensation = 1 / std::sqrt(drive);
        level = dbToGain(smoothed[Level] * 24 - 12);
    }
    double tick(double input, int channel) {
        auto &c = channels[channel];
        if (!std::isfinite(input))
            input = 0;
        input = std::clamp(input, -32.0, 32.0);
        const double dry = c.delayed(input);
        const double pre = c.preMid.tick(c.inputCut.tick(input));
        double wet = c.oversampler.tick(pre, [this](double x) {
            const double z = x * drive;
            const double soft = std::tanh(z);
            const double a = z / 1.6, a2 = a * a;
            const double led = z / std::sqrt(std::sqrt(1 + a2 * a2));
            constexpr double bias = .24491866240370913; // tanh(0.25)
            const double asym = (std::tanh(z + .25) - bias) / (1 - bias * bias);
            return (soft * clipWeights[0] + led * clipWeights[1] + asym * clipWeights[2]) *
                   compensation;
        });
        const double dc = wet - c.dcX + dcR * c.dcY;
        c.dcX = wet;
        c.dcY = std::abs(dc) < 1.e-25 ? 0 : dc;
        wet = c.treble.tick(c.mid.tick(c.bass.tick(c.tone.tick(dc)))) * level;
        const double y = wet * (1 - smoothed[Bypass]) + dry * smoothed[Bypass];
        return std::isfinite(y) ? y : 0;
    }

  private:
    void updateFilters() {
        for (auto &c : channels) {
            c.inputCut.highpass(sr, 30 + 105 * smoothed[Voice]);
            c.preMid.peak(sr, 850, .72, 4.5 * smoothed[Voice]);
            c.tone.lowpass(sr, 2200 * std::pow(8.0, smoothed[Tone]));
            c.bass.shelf(sr, 120, 24 * smoothed[Bass] - 12, false);
            c.mid.peak(sr, 750, .75, 24 * smoothed[Mid] - 12);
            c.treble.shelf(sr, 3200, 24 * smoothed[Treble] - 12, true);
        }
    }
    std::array<Channel, 2> channels;
    Values target = defaults(), smoothed = defaults();
    std::array<double, 3> clipWeights{1, 0, 0};
    double sr = 48000, smoothing = .001, dcR = .999, drive = 1, compensation = 1, level = 1;
    uint64_t counter = 0;
};
} // namespace sundrive
