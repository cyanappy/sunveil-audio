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
namespace glassgarden {
using namespace Steinberg;
using namespace Steinberg::Vst;
DECLARE_UID(processorUID, 0x09B4F80A, 0x0BF04FBA, 0x9A218B87, 0x3AD15A47)
DECLARE_UID(controllerUID, 0x88913B6C, 0x10ED4371, 0xA9AD9A71, 0x1D129AFF)
inline bool sameID(const void *a, const void *b) {
    return a && b && std::memcmp(a, b, 16) == 0;
}
inline void text16(char16 *out, const char *text, size_t capacity = 128) {
    size_t i = 0;
    for (; i + 1 < capacity && text[i]; ++i)
        out[i] = static_cast<unsigned char>(text[i]);
    out[i] = 0;
}
} // namespace glassgarden
