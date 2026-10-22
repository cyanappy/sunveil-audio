#pragma once
#include "Dsp.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/gui/iplugview.h"
#include <atomic>
#include <cstring>
namespace afterglow {
using namespace Steinberg;
using namespace Steinberg::Vst;
DECLARE_UID(processorUID, 0x2D1AC732, 0xE0494D75, 0xBCB17B30, 0x633632F1)
DECLARE_UID(controllerUID, 0xAB4175FD, 0x63724684, 0xA6E3E91F, 0xA1230E5B)
inline bool sameID(const void *a, const void *b) {
    return a && b && std::memcmp(a, b, 16) == 0;
}
inline void text16(char16 *out, const char *text, size_t capacity = 128) {
    size_t i = 0;
    for (; i + 1 < capacity && text[i]; ++i)
        out[i] = static_cast<unsigned char>(text[i]);
    out[i] = 0;
}
} // namespace afterglow
