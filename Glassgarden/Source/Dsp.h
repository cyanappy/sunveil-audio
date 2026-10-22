#pragma once
#include "Parameters.h"
#include <vector>
#include <limits>

namespace glassgarden {
// Chronological addressing keeps reads valid across ring wraps and an O(1)
// clear. Old storage is never audible after clear(), even before it is replaced.
class CaptureBuffer {
public:
    void prepare(size_t length) {
        capacity = std::max<size_t>(64, length);
        for (auto& channel : samples) channel.assign(capacity, 0.0);
        clear();
    }
    void clear() noexcept { total = 0; filled = 0; write = 0; }
    void push(double left, double right) noexcept {
        samples[0][write] = left; samples[1][write] = right;
        if (++write == capacity) write = 0;
        ++total; if (filled < capacity) ++filled;
    }
    uint64_t end() const noexcept { return total; }
    size_t valid() const noexcept { return filled; }
    void read(double position, double& left, double& right) const noexcept {
        left = right = 0.0;
        if (!std::isfinite(position) || position < 0 || filled < 8) return;
        const auto base = int64_t(std::floor(position));
        if (base - 1 < int64_t(total - filled) || base + 2 >= int64_t(total)) return;
        const double t = position - double(base);
        const size_t b = size_t(base) % capacity;
        const size_t a = b ? b - 1 : capacity - 1;
        const size_t c = b + 1 < capacity ? b + 1 : 0;
        const size_t d = c + 1 < capacity ? c + 1 : 0;
        auto interpolate = [&](const std::vector<double>& v) {
            return v[b] + .5 * t * (v[c] - v[a] + t *
                (2*v[a] - 5*v[b] + 4*v[c] - v[d] + t * (3*(v[b]-v[c]) + v[d]-v[a])));
        };
        left = interpolate(samples[0]); right = interpolate(samples[1]);
    }
private:
    std::array<std::vector<double>, 2> samples;
    size_t capacity = 0, write = 0, filled = 0;
    uint64_t total = 0;
};

class Engine {
public:
    static constexpr unsigned latency = 0;
    static constexpr unsigned maxGrains = 96;
    void prepare(double rate, const Values& v) {
        fs = std::isfinite(rate) ? std::clamp(rate, 22050.0, 384000.0) : 48000.0;
        capture.prepare(size_t(std::ceil(fs * 6.0)) + 32);
        setAll(v); smooth = target;
        smoothing = 1.0 - std::exp(-1.0 / (fs * .012));
        gateSmoothing = 1.0 - std::exp(-1.0 / (fs * .004));
        gate = target[Bypass] >= .5 ? 0.0 : 1.0;
        driftC = std::cos(2*pi*.31/fs); driftS = std::sin(2*pi*.31/fs);
        clear(); updateControls();
    }
    void setAll(const Values& values) noexcept {
        for (unsigned i = 0; i < Count; ++i) target[i] = canonical(i, values[i]);
    }
    void set(unsigned id, double value) noexcept {
        if (id < Count) target[id] = canonical(id, value);
    }
    void setTempo(double value) noexcept {
        if (std::isfinite(value) && value >= 10 && value <= 999) tempo = value;
    }
    void clear() noexcept {
        capture.clear();
        for (auto& g : grains) g = {};
        inputPrevious = {}; highState = {}; wetState = {}; feedbackLast = {};
        countdown = 0; controlCounter = 0;
        driftCos = 1; driftSin = 0;
        randomState = 0x739a21bdu;
    }
    void process(double left, double right, double& outLeft, double& outRight) noexcept {
        left = sanitize(left); right = sanitize(right);
        for (unsigned i = 0; i < Count; ++i)
            if (!definitions[i].steps) smooth[i] += smoothing * (target[i] - smooth[i]);
        if (--controlCounter <= 0) { updateControls(); controlCounter = 32; }
        const double in[2] = {left, right};
        const bool hold = target[Freeze] >= .5;
        if (!hold) {
            double high[2]{};
            for (int c = 0; c < 2; ++c) {
                highState[c] = highPole * (highState[c] + in[c] - inputPrevious[c]);
                inputPrevious[c] = in[c];
                if (std::abs(highState[c]) < 1.e-25) highState[c] = 0;
                high[c] = highState[c];
            }
            const double l = high[0] + feedback * (.82*feedbackLast[0] + .18*feedbackLast[1]);
            const double r = high[1] + feedback * (.82*feedbackLast[1] + .18*feedbackLast[0]);
            capture.push(ceiling(l), ceiling(r));
        }
        if (--countdown <= 0) {
            launch(0.0, 1.0);
            const int harmony = int(toPlain(Harmony, target[Harmony]));
            if (harmony) launch(harmony == 1 ? 7.0 : 12.0, .58);
            const double jitter = target[Sync] >= .5 ? 1.0 : 1.0 + .22*smooth[Scatter]*randomSigned();
            countdown += std::max(1.0, interval * jitter);
        }
        double wet[2]{}, weight = 0;
        const double vibrato = 1.0 + .003*smooth[Drift]*driftSin;
        for (auto& grain : grains) {
            if (!grain.remaining) continue;
            double l, r; capture.read(grain.position, l, r);
            double window = std::max(0.0, .5 - .5*grain.windowCos);
            if (grain.texture == 0) window = std::sqrt(window);
            if (grain.texture == 2) window = window*window*window;
            const double amplitude = window * grain.amplitude;
            wet[0] += l*amplitude*grain.panLeft;
            wet[1] += r*amplitude*grain.panRight;
            weight += amplitude;
            grain.position += grain.step * vibrato;
            const double nextCos = grain.windowCos*grain.cosStep - grain.windowSin*grain.sinStep;
            grain.windowSin = grain.windowSin*grain.cosStep + grain.windowCos*grain.sinStep;
            grain.windowCos = nextCos;
            --grain.remaining;
        }
        const double normalization = std::max(1.0, .78*weight);
        for (int c = 0; c < 2; ++c) {
            wet[c] /= normalization;
            wetState[c] += lowCoefficient * (wet[c] - wetState[c]);
            if (std::abs(wetState[c]) < 1.e-25) wetState[c] = 0;
            wet[c] = ceiling(wetState[c]);
            feedbackLast[c] = wet[c];
        }
        const double mid = (wet[0]+wet[1])*.5;
        const double side = (wet[0]-wet[1])*.5*width;
        wet[0] = mid+side; wet[1] = mid-side;
        const double wantedGate = target[Bypass] >= .5 ? 0.0 : 1.0;
        gate += gateSmoothing*(wantedGate-gate);
        if (std::abs(gate-wantedGate) < 1.e-12) gate = wantedGate;
        const double processedLeft = ceiling((left*dryGain + wet[0]*wetGain) * outputGain);
        const double processedRight = ceiling((right*dryGain + wet[1]*wetGain) * outputGain);
        outLeft = left + gate*(processedLeft-left);
        outRight = right + gate*(processedRight-right);
        const double nextCos = driftCos*driftC - driftSin*driftS;
        driftSin = driftSin*driftC + driftCos*driftS; driftCos = nextCos;
    }
    unsigned activeGrains() const noexcept {
        unsigned n = 0; for (const auto& g : grains) if (g.remaining) ++n; return n;
    }
    size_t capturedSamples() const noexcept { return capture.valid(); }
private:
    struct Grain {
        double position = 0, step = 1, amplitude = 1, panLeft = 1, panRight = 1;
        double windowCos = 1, windowSin = 0, cosStep = 1, sinStep = 0;
        unsigned remaining = 0;
        int texture = 1;
    };
    static double sanitize(double value) noexcept {
        return std::isfinite(value) ? std::clamp(value, -16.0, 16.0) : 0.0;
    }
    static double ceiling(double value) noexcept {
        if (!std::isfinite(value)) return 0;
        const double magnitude = std::abs(value);
        if (magnitude <= 1.0) return value;
        return std::copysign(1.0 + .25*std::tanh((magnitude-1.0)*4.0), value);
    }
    double randomUnit() noexcept {
        randomState ^= randomState << 13; randomState ^= randomState >> 17; randomState ^= randomState << 5;
        return double(randomState) / 4294967296.0;
    }
    double randomSigned() noexcept { return randomUnit()*2.0-1.0; }
    void updateControls() noexcept {
        pitchRatio = std::pow(2.0, toPlain(Pitch, smooth[Pitch])/12.0);
        grainSamples = fs*toPlain(GrainSize, smooth[GrainSize])*.001;
        interval = fs/toPlain(Density, smooth[Density]);
        if (target[Sync] >= .5) interval = fs*60.0/tempo*beatLengths[int(toPlain(Division, target[Division]))];
        feedback = smooth[Feedback]*.88;
        highPole = std::exp(-2*pi*toPlain(LowCut, smooth[LowCut])/fs);
        lowCoefficient = 1.0 - std::exp(-2*pi*std::min(fs*.42, toPlain(Tone, smooth[Tone]))/fs);
        width = 1.5*smooth[Width];
        dryGain = std::cos(smooth[Mix]*pi*.5); wetGain = std::sin(smooth[Mix]*pi*.5);
        if (smooth[Mix] <= 1.e-12) { dryGain = 1; wetGain = 0; }
        if (smooth[Mix] >= 1.0-1.e-12) dryGain = 0;
        outputGain = dbToGain(toPlain(Output, smooth[Output]));
    }
    void launch(double semitones, double amplitude) noexcept {
        if (capture.valid() < 64) return;
        Grain* slot = nullptr;
        for (auto& g : grains) if (!g.remaining) { slot = &g; break; }
        if (!slot) return; // Never cut a sounding grain to make room for another.
        const double cents = smooth[Drift]*30.0*randomSigned();
        const double rate = pitchRatio * std::pow(2.0, semitones/12.0+cents/1200.0);
        const int texture = int(toPlain(Texture, target[Texture]));
        const double variation = texture == 2 ? .7 + .6*randomUnit() : 1.0;
        const double available = double(capture.valid());
        if (available < 32 + 16*rate*1.02) return;
        const unsigned length = unsigned(std::max(16.0, std::min(grainSamples*variation, (available-32)/(rate*1.02))));
        const double travel = rate*1.01*length;
        const double oldest = double(capture.end()-capture.valid());
        const double pre = std::min(fs*.3*smooth[Predelay], std::max(0.0, available-travel-12));
        const double anchor = double(capture.end())-5-pre;
        const double room = std::max(0.0, anchor-travel-oldest-3);
        const double lookback = randomUnit()*std::min(room, fs*1.6*smooth[Scatter]);
        const bool reverse = target[Reverse] >= .5;
        const double pan = randomSigned()*smooth[Scatter];
        *slot = {};
        slot->position = anchor-lookback-(reverse ? 0.0 : travel);
        slot->step = reverse ? -rate : rate;
        slot->remaining = length;
        slot->texture = texture;
        slot->amplitude = amplitude;
        slot->panLeft = std::sqrt(1.0-pan);
        slot->panRight = std::sqrt(1.0+pan);
        slot->cosStep = std::cos(2*pi/double(length));
        slot->sinStep = std::sin(2*pi/double(length));
    }
    double fs = 48000, tempo = 120, smoothing = .001, gateSmoothing = .005;
    Values target = defaults(), smooth = defaults();
    CaptureBuffer capture;
    std::array<Grain, maxGrains> grains{};
    std::array<double, 2> inputPrevious{}, highState{}, wetState{}, feedbackLast{};
    double pitchRatio = 2, grainSamples = 5520, interval = 1600, countdown = 0;
    double feedback = .28, highPole = .99, lowCoefficient = .5, width = 1.1;
    double dryGain = 1, wetGain = 0, outputGain = 1, gate = 1;
    double driftCos = 1, driftSin = 0, driftC = 1, driftS = 0;
    uint32_t randomState = 0x739a21bdu;
    int controlCounter = 0;
};
} // namespace glassgarden
