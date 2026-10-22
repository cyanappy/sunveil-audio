#pragma once
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>

namespace sundrive {
constexpr const char *kName = "Sundrive";
constexpr const char *kVersion = "1.3.2";
constexpr const char *kVendor = "Sunveil Audio";
enum Param : unsigned { Drive, Tone, Level, Bass, Mid, Treble, Voice, Clipping, Bypass, Count };
using Values = std::array<double, Count>;
inline double clamp01(double v) {
    return std::isfinite(v) ? (v < 0 ? 0 : v > 1 ? 1 : v) : 0;
}
inline double dbToGain(double d) {
    return std::pow(10.0, d / 20.0);
}
inline double gainToDb(double g) {
    return 20.0 * std::log10(g > 1.e-8 ? g : 1.e-8);
}
struct Definition {
    const char *name;
    const char *unit;
    int steps;
    double initial;
};
constexpr Definition definitions[Count] = {
    {"Drive", "%", 0, .20}, {"Tone", "%", 0, .65},  {"Level", "dB", 0, .50},
    {"Bass", "dB", 0, .50}, {"Mid", "dB", 0, .50},  {"Treble", "dB", 0, .50},
    {"Voice", "", 1, 0},    {"Clipping", "", 2, 0}, {"Bypass", "", 1, 0}};
constexpr const char *voices[] = {"Transparent", "Mid Push"};
constexpr const char *clippings[] = {"Soft", "LED", "Asymmetric"};
struct Preset {
    const char *name;
    Values values;
};
constexpr Preset presets[] = {{"Clear Edge", {.20, .65, .50, .50, .50, .50, 0, 0, 0}},
                              {"Mid Push", {.35, .55, .56, .42, .58, .50, 1, 0, 0}},
                              {"Glass Boost", {.06, .80, .67, .50, .50, .54, 0, .5, 0}},
                              {"Indie Crunch", {.58, .62, .50, .46, .56, .54, 0, 1, 0}},
                              {"Tight Lead", {.67, .58, .56, .40, .625, .50, 1, .5, 0}},
                              {"Warm Stack", {.40, .35, .50, .54, .56, .46, 0, 0, 0}}};
constexpr int presetCount = sizeof(presets) / sizeof(presets[0]);
inline Values defaults() {
    return presets[0].values;
}
inline double canonical(unsigned id, double n) {
    n = clamp01(n);
    return id < Count && definitions[id].steps
               ? std::round(n * definitions[id].steps) / definitions[id].steps
               : n;
}
inline double toPlain(unsigned id, double n) {
    n = clamp01(n);
    if (id >= Level && id <= Treble)
        return n * 24 - 12;
    if (id == Voice || id == Bypass)
        return std::round(n);
    if (id == Clipping)
        return std::round(n * 2);
    return n * 100;
}
inline double fromPlain(unsigned id, double v) {
    return canonical(id, id >= Level && id <= Treble   ? (v + 12) / 24
                         : id == Clipping              ? v / 2
                         : id == Voice || id == Bypass ? v
                                                       : v / 100);
}
inline void format(unsigned id, double n, char *out, size_t size, bool units = false) {
    n = canonical(id, n);
    if (id == Voice)
        std::snprintf(out, size, "%s", voices[n >= .5]);
    else if (id == Clipping)
        std::snprintf(out, size, "%s", clippings[static_cast<int>(std::round(n * 2))]);
    else if (id == Bypass)
        std::snprintf(out, size, "%s", n >= .5 ? "Bypassed" : "Engaged");
    else if (id >= Level && id <= Treble)
        std::snprintf(out, size, units ? "%+.1f dB" : "%+.1f", toPlain(id, n));
    else
        std::snprintf(out, size, units ? "%.0f %%" : "%.0f", n * 100);
}
} // namespace sundrive
