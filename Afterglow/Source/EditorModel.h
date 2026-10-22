#pragma once
#include "Artwork.h"

namespace afterglow::ui {
// Keep fractional movement and the drag's own value. Sensitivity follows the
// editor size, rather than rounding each mouse position into artwork pixels.
class KnobDrag {
  public:
    void begin(double value, double clientY, int editorWidth) {
        current = clamp01(value);
        lastY = clientY;
        sensitivity = 600.0 / std::max(1, editorWidth) / 240.0;
    }
    double move(double clientY, bool fine) {
        current = clamp01(current + (lastY - clientY) * sensitivity * (fine ? .1 : 1.0));
        lastY = clientY;
        return current;
    }

  private:
    double current = 0, lastY = 0, sensitivity = 1.0 / 240.0;
};

inline artwork::Box knobBounds(const artwork::Knob& k, bool small = false) {
    const int offset = small ? 81 : k.id >= Decay ? 105 : 116;
    return {k.x - 110, k.y - k.r - 9, 220, k.r + 9 + offset + 26};
}
inline artwork::Box padded(artwork::Box b, int pad = 3) {
    return {b.x - pad, b.y - pad, b.w + pad * 2, b.h + pad * 2};
}
struct Damage {
    std::array<artwork::Box, Count * 4 + 12> boxes{};
    unsigned count = 0;
    void add(artwork::Box b) {
        if (count < boxes.size()) boxes[count++] = b;
    }
    void parameter(unsigned id, bool advanced) {
        using namespace artwork;
        for (auto k : knobs) if (k.id == id) { add(knobBounds(k)); return; }
        if (advanced) {
            for (auto k : advancedKnobs) if (k.id == id) { add(knobBounds(k, true)); return; }
            switch (id) {
            case Routing: add(padded(routeBox)); break;
            case Sync: add(padded(syncBox)); break;
            case Division: add(padded(divisionBox)); break;
            case Trails: add(padded(trailsBox)); break;
            case Bypass: add(padded(bypassBox)); break;
            default: break;
            }
        } else {
            if (id == DelayType) add({282, 700, 86, 88});
            if (id == ReverbType) add({659, 700, 86, 88});
            if (id == DelayOn || id == Bypass) add({235, 1172, 50, 50});
            if (id == Freeze || id == Bypass) add({487, 1172, 50, 50});
            if (id == ReverbOn || id == Bypass) add({741, 1172, 50, 50});
        }
        if (id == Bypass) add({358, 1456, 308, 43});
        // Sync and its note division also change the main Time readout.
        if (id == Sync || id == Division) add(knobBounds(knobs[0]));
    }
    void focus(int id, bool advanced) {
        using namespace artwork;
        if (id < 0) return;
        for (auto k : knobs) if (int(k.id) == id) { add(knobBounds(k)); return; }
        if (advanced) {
            for (auto k : advancedKnobs) if (int(k.id) == id) add(knobBounds(k, true));
        } else {
            if (id == DelayType || id == ReverbType) parameter(unsigned(id), false);
            for (auto k : switches) if (int(k.id) == id)
                add({k.x - k.r - 5, k.y - k.r - 5, (k.r + 5) * 2, (k.r + 5) * 2});
        }
    }
};
inline Damage changes(const artwork::Pedal& before, const artwork::Pedal& after) {
    Damage damage;
    if (before.advanced != after.advanced) {
        damage.add({0, 0, artwork::W, artwork::H});
        return damage;
    }
    for (unsigned i = 0; i < Count; ++i)
        if (before.values[i] != after.values[i]) damage.parameter(i, after.advanced);
    if (before.presetIndex != after.presetIndex) damage.add(padded(artwork::presetBox));
    if (before.hasFocus != after.hasFocus || before.focused != after.focused) {
        if (before.hasFocus) damage.focus(before.focused, before.advanced);
        if (after.hasFocus) damage.focus(after.focused, after.advanced);
    }
    if (before.tapActive != after.tapActive) damage.add(padded(artwork::tapBox));
    if (before.clearActive != after.clearActive && after.advanced)
        damage.add(padded(artwork::clearBox));
    return damage;
}
} // namespace afterglow::ui
