#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>

namespace glassgarden {
constexpr const char* kName = "Glassgarden";
constexpr const char* kVersion = "1.0.0";
constexpr const char* kVendor = "Sunveil Audio";
constexpr double pi = 3.14159265358979323846;
enum Param : unsigned {
    Pitch, GrainSize, Scatter, Density, Feedback, Mix, Texture, Harmony,
    Bypass, Freeze, Reverse, Tone, Drift, Width, LowCut, Predelay, Output,
    Sync, Division, Count
};
using Values = std::array<double, Count>;
inline double clamp01(double v) { return std::isfinite(v) ? std::clamp(v, 0.0, 1.0) : 0.0; }
inline double dbToGain(double v) { return std::pow(10.0, v / 20.0); }
inline double gainToDb(double v) { return 20.0 * std::log10(std::max(1.e-12, v)); }
struct Definition { const char* name; const char* unit; int steps; double initial; };
inline const Definition definitions[Count] = {
    {"Pitch", "st", 0, .75},
    {"Grain size", "ms", 0, std::log(115.0 / 15.0) / std::log(350.0 / 15.0)},
    {"Scatter", "%", 0, .38},
    {"Density", "grains/s", 0, std::log(30.0 / 2.0) / std::log(45.0)},
    {"Feedback", "%", 0, 28.0 / 88.0}, {"Mix", "%", 0, .42},
    {"Texture", "", 2, .5}, {"Harmony", "", 2, 0},
    {"Bypass", "", 1, 0}, {"Freeze", "", 1, 0}, {"Reverse", "", 1, 0},
    {"Tone", "Hz", 0, std::log(8000.0 / 800.0) / std::log(20.0)},
    {"Drift", "%", 0, .12}, {"Width", "%", 0, 110.0 / 150.0},
    {"Low cut", "Hz", 0, std::log(85.0 / 20.0) / std::log(40.0)},
    {"Pre-delay", "ms", 0, 15.0 / 300.0}, {"Output", "dB", 0, .75},
    {"Tempo sync", "", 1, 0}, {"Note division", "", 4, .5}
};
inline constexpr const char* textures[] = {"Mist", "Glass", "Shards"};
inline constexpr const char* harmonies[] = {"Solo", "Fifth", "Octave"};
inline constexpr const char* divisions[] = {"1/4", "1/8", "1/16", "1/32", "1/8 triplet"};
inline constexpr double beatLengths[] = {1.0, .5, .25, .125, 1.0 / 3.0};
inline double canonical(unsigned id, double value) {
    value = clamp01(value);
    return id < Count && definitions[id].steps ?
        std::round(value * definitions[id].steps) / definitions[id].steps : value;
}
inline double toPlain(unsigned id, double n) {
    if (id >= Count) return 0;
    n = canonical(id, n);
    switch (id) {
    case Pitch: return -24.0 + 48.0 * n;
    case GrainSize: return 15.0 * std::pow(350.0 / 15.0, n);
    case Density: return 2.0 * std::pow(45.0, n);
    case Feedback: return 88.0 * n;
    case Tone: return 800.0 * std::pow(20.0, n);
    case Width: return 150.0 * n;
    case LowCut: return 20.0 * std::pow(40.0, n);
    case Predelay: return 300.0 * n;
    case Output: return -18.0 + 24.0 * n;
    default: return definitions[id].steps ? std::round(n * definitions[id].steps) : n * 100.0;
    }
}
inline double fromPlain(unsigned id, double value) {
    if (id >= Count || !std::isfinite(value)) return 0;
    switch (id) {
    case Pitch: return clamp01((value + 24.0) / 48.0);
    case GrainSize: return clamp01(std::log(std::max(15.0, value) / 15.0) / std::log(350.0 / 15.0));
    case Density: return clamp01(std::log(std::max(2.0, value) / 2.0) / std::log(45.0));
    case Feedback: return clamp01(value / 88.0);
    case Tone: return clamp01(std::log(std::max(800.0, value) / 800.0) / std::log(20.0));
    case Width: return clamp01(value / 150.0);
    case LowCut: return clamp01(std::log(std::max(20.0, value) / 20.0) / std::log(40.0));
    case Predelay: return clamp01(value / 300.0);
    case Output: return clamp01((value + 18.0) / 24.0);
    default: return canonical(id, definitions[id].steps ? value / definitions[id].steps : value / 100.0);
    }
}
inline void format(unsigned id, double n, char* out, size_t length, bool units = false) {
    if (id >= Count) { if (length) out[0] = 0; return; }
    n = canonical(id, n);
    const int state = int(std::round(n * definitions[id].steps));
    if (id == Texture) std::snprintf(out, length, "%s", textures[state]);
    else if (id == Harmony) std::snprintf(out, length, "%s", harmonies[state]);
    else if (id == Division) std::snprintf(out, length, "%s", divisions[state]);
    else if (id == Sync) std::snprintf(out, length, "%s", state ? "Host tempo" : "Free density");
    else if (id == Bypass) std::snprintf(out, length, "%s", state ? "Bypassed" : "Engaged");
    else if (id == Freeze) std::snprintf(out, length, "%s", state ? "Frozen" : "Live");
    else if (id == Reverse) std::snprintf(out, length, "%s", state ? "Reverse" : "Forward");
    else if (id == Pitch) std::snprintf(out, length, units ? "%+.1f st" : "%+.1f", toPlain(id, n));
    else if (id == GrainSize || id == Predelay) std::snprintf(out, length, units ? "%.0f ms" : "%.0f", toPlain(id, n));
    else if (id == Density) std::snprintf(out, length, units ? "%.1f /s" : "%.1f", toPlain(id, n));
    else if (id == Tone || id == LowCut) std::snprintf(out, length, units ? "%.0f Hz" : "%.0f", toPlain(id, n));
    else if (id == Output) std::snprintf(out, length, units ? "%+.1f dB" : "%+.1f", toPlain(id, n));
    else std::snprintf(out, length, units ? "%.0f %%" : "%.0f", toPlain(id, n));
}
struct Preset { const char* name; Values values; };
inline Values makePreset(double pitch, double grain, double scatter, double density,
                         double feedback, double mix, int texture, int harmony,
                         double tone, double drift, double width, bool reverse = false) {
    Values v{};
    for (unsigned i = 0; i < Count; ++i) v[i] = definitions[i].initial;
    v[Pitch] = fromPlain(Pitch, pitch); v[GrainSize] = fromPlain(GrainSize, grain);
    v[Scatter] = scatter; v[Density] = fromPlain(Density, density);
    v[Feedback] = fromPlain(Feedback, feedback); v[Mix] = mix;
    v[Texture] = texture / 2.0; v[Harmony] = harmony / 2.0;
    v[Tone] = fromPlain(Tone, tone); v[Drift] = drift;
    v[Width] = fromPlain(Width, width); v[Reverse] = reverse ? 1 : 0;
    return v;
}
inline const Preset presets[] = {
    {"Crystal Canopy", makePreset(12, 115, .38, 30, 28, .42, 1, 0, 8000, .12, 110)},
    {"Glass Petals", makePreset(7, 68, .60, 22, 22, .40, 1, 0, 9800, .10, 125)},
    {"Prism Cathedral", makePreset(0, 180, .68, 34, 55, .55, 0, 2, 6800, .24, 140)},
    {"Reverse Ferns", makePreset(-12, 210, .55, 16, 40, .62, 1, 1, 6200, .18, 115, true)},
    {"Broken Sunlight", makePreset(12, 30, .85, 12, 23, .60, 2, 0, 11500, .36, 145)},
    {"Weightless", makePreset(0, 285, .70, 48, 56, .45, 0, 2, 5700, .32, 135)}
};
inline constexpr int presetCount = int(sizeof(presets) / sizeof(presets[0]));
inline Values defaults() { return presets[0].values; }
} // namespace glassgarden
