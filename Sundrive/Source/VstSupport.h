#pragma once
#include "Dsp.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/gui/iplugview.h"
#include <atomic>
#include <cstring>
namespace sundrive {
using namespace Steinberg;
using namespace Steinberg::Vst;
DECLARE_UID(processorUID, 0x548C4D8C, 0x29534B77, 0xAE351D35, 0xD9468AE7)
DECLARE_UID(controllerUID, 0x62AEB56C, 0x786E4CF4, 0x9FD2E2F7, 0x607DAC71)
inline bool sameID(const void *a, const void *b) {
    return a && b && std::memcmp(a, b, 16) == 0;
}
inline void text16(char16 *out, const char *text, size_t capacity = 128) {
    size_t i = 0;
    for (; i + 1 < capacity && text[i]; ++i)
        out[i] = static_cast<unsigned char>(text[i]);
    out[i] = 0;
}
} // namespace sundrive
