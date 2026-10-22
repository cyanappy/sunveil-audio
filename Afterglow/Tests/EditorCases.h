#pragma once
#include "../Source/EditorModel.h"
#include <string>
#include <vector>

namespace afterglow::ui {
struct EditorCase { std::string name; artwork::Pedal before, after; };
inline std::vector<EditorCase> editorCases() {
    std::vector<EditorCase> cases;
    for (bool advanced : {false, true}) {
        artwork::Pedal base;
        base.advanced=advanced;
        for (unsigned id=0; id<Count; ++id) {
            auto after=base;
            after.values[id]=base.values[id]<.5 ? 1 : 0;
            if (id!=Bypass && id!=Freeze && id!=DelayOn && id!=ReverbOn) after.presetIndex=-1;
            const auto name=std::string(advanced ? "details-" : "main-")+std::to_string(id);
            cases.push_back({name,base,after});
            cases.push_back({name+"-return",after,base});
        }
        for (unsigned id=0; id<Count; ++id) {
            auto after=base;
            after.hasFocus=true; after.focused=int(id);
            const auto name=std::string(advanced ? "details-focus-" : "focus-")+std::to_string(id);
            cases.push_back({name,base,after});
            cases.push_back({name+"-leave",after,base});
        }
        auto after=base;
        after.values[Sync]=1; after.values[Division]=.75;
        cases.push_back({advanced ? "details-sync" : "main-sync",base,after});
        after=base; after.values[Bypass]=1; after.values[Freeze]=1;
        after.hasFocus=true; after.focused=Freeze;
        cases.push_back({advanced ? "details-bypass-freeze" : "bypass-freeze",base,after});
        after=base; after.tapActive=true;
        cases.push_back({advanced ? "details-tap" : "tap",base,after});
        after=base; after.clearActive=true;
        cases.push_back({advanced ? "details-clear" : "clear",base,after});
    }
    artwork::Pedal base,after;
    after.advanced=true;
    cases.push_back({"open-details",base,after});
    cases.push_back({"close-details",after,base});
    after=base; after.values=presets[2].values; after.presetIndex=2;
    cases.push_back({"load-preset",base,after});
    return cases;
}
} // namespace afterglow::ui
