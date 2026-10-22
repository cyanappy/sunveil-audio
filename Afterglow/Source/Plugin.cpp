#include "Plugin.h"
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <limits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
HINSTANCE afterglowModule = nullptr;
BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        afterglowModule = h;
        DisableThreadLibraryCalls(h);
    }
    return TRUE;
}
#endif

namespace afterglow {
namespace {
constexpr uint32 stateMagic = 0x31474641; // AFG1, little endian
bool writeExact(IBStream *s, const void *p, int32 bytes) {
    int32 n = 0;
    return s && s->write(const_cast<void *>(p), bytes, &n) == kResultOk && n == bytes;
}
bool readExact(IBStream *s, void *p, int32 bytes) {
    int32 n = 0;
    return s && s->read(p, bytes, &n) == kResultOk && n == bytes;
}
bool readValues(IBStream *s, Values &v) {
    uint32 header[3]{};
    if (!readExact(s, header, sizeof(header)) || header[0] != stateMagic || header[1] != 1 ||
        header[2] != Count)
        return false;
    Values temp{};
    if (!readExact(s, temp.data(), sizeof(double) * Count))
        return false;
    for (unsigned i = 0; i < Count; ++i) {
        if (!std::isfinite(temp[i]))
            return false;
        temp[i] = canonical(i, temp[i]);
    }
    v = temp;
    return true;
}
bool writeValues(IBStream *s, const Values &v) {
    const uint32 header[] = {stateMagic, 1, Count};
    return writeExact(s, header, sizeof(header)) && writeExact(s, v.data(), sizeof(double) * Count);
}
bool directionValid(BusDirection d) {
    return d == kInput || d == kOutput;
}
} // namespace

Processor::Processor() {
    auto v = defaults();
    for (unsigned i = 0; i < Count; ++i)
        parameters[i].store(v[i]);
    engine.prepare(sampleRate, v);
}
Processor::~Processor() {
    if (peer)
        peer->release();
}
Values Processor::snapshot() const {
    Values v;
    for (unsigned i = 0; i < Count; ++i)
        v[i] = parameters[i].load(std::memory_order_relaxed);
    return v;
}
tresult PLUGIN_API Processor::queryInterface(const TUID id, void **out) {
    if (!out)
        return kInvalidArgument;
    *out = nullptr;
    if (sameID(id, FUnknown_iid) || sameID(id, IComponent_iid) || sameID(id, IPluginBase_iid))
        *out = static_cast<IComponent *>(this);
    else if (sameID(id, IAudioProcessor_iid))
        *out = static_cast<IAudioProcessor *>(this);
    else if (sameID(id, IConnectionPoint_iid))
        *out = static_cast<IConnectionPoint *>(this);
    else if (sameID(id, IProcessContextRequirements_iid))
        *out = static_cast<IProcessContextRequirements *>(this);
    if (!*out)
        return kNoInterface;
    addRef();
    return kResultOk;
}
tresult PLUGIN_API Processor::initialize(FUnknown *) {
    if (initialized)
        return kResultFalse;
    initialized = true;
    return kResultOk;
}
tresult PLUGIN_API Processor::terminate() {
    initialized = false;
    if (peer) {
        auto p = peer;
        peer = nullptr;
        p->release();
    }
    return kResultOk;
}
tresult PLUGIN_API Processor::getControllerClassId(TUID id) {
    std::memcpy(id, controllerUID, 16);
    return kResultOk;
}
int32 PLUGIN_API Processor::getBusCount(MediaType t, BusDirection d) {
    return t == kAudio && directionValid(d) ? 1 : 0;
}
tresult PLUGIN_API Processor::getBusInfo(MediaType t, BusDirection d, int32 index, BusInfo &b) {
    if (t != kAudio || !directionValid(d) || index != 0)
        return kInvalidArgument;
    b = {};
    b.mediaType = kAudio;
    b.direction = d;
    b.channelCount = d == kInput ? inputChannels : outputChannels;
    b.busType = kMain;
    b.flags = BusInfo::kDefaultActive;
    text16(b.name, d == kInput ? "Guitar In" : "Pedal Out");
    return kResultOk;
}
tresult PLUGIN_API Processor::activateBus(MediaType t, BusDirection d, int32 i, TBool v) {
    if (t != kAudio || i != 0 || !directionValid(d))
        return kInvalidArgument;
    (d == kInput ? inputActive : outputActive) = v != 0;
    return kResultOk;
}
tresult PLUGIN_API Processor::setActive(TBool v) {
    if (v)
        engine.prepare(sampleRate, snapshot());
    return kResultOk;
}
tresult PLUGIN_API Processor::setState(IBStream *s) {
    Values v;
    if (!readValues(s, v))
        return kResultFalse;
    for (unsigned i = 0; i < Count; ++i)
        parameters[i].store(v[i], std::memory_order_relaxed);
    return kResultOk;
}
tresult PLUGIN_API Processor::getState(IBStream *s) {
    return writeValues(s, snapshot()) ? kResultOk : kResultFalse;
}
tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement *in, int32 ni,
                                                 SpeakerArrangement *out, int32 no) {
    if (!in || !out || ni != 1 || no != 1)
        return kResultFalse;
    const int ic = in[0] == SpeakerArr::kMono ? 1 : in[0] == SpeakerArr::kStereo ? 2 : 0;
    const int oc = out[0] == SpeakerArr::kMono ? 1 : out[0] == SpeakerArr::kStereo ? 2 : 0;
    if (!ic || !oc || (ic == 2 && oc == 1))
        return kResultFalse;
    inputChannels = ic;
    outputChannels = oc;
    return kResultOk;
}
tresult PLUGIN_API Processor::getBusArrangement(BusDirection d, int32 i, SpeakerArrangement &a) {
    if (i || !directionValid(d))
        return kInvalidArgument;
    a = (d == kInput ? inputChannels : outputChannels) == 1 ? SpeakerArr::kMono
                                                            : SpeakerArr::kStereo;
    return kResultOk;
}
tresult PLUGIN_API Processor::setupProcessing(ProcessSetup &s) {
    if (!std::isfinite(s.sampleRate) || s.sampleRate < 22050 || s.sampleRate > 384000 ||
        s.maxSamplesPerBlock < 0 || canProcessSampleSize(s.symbolicSampleSize) != kResultTrue)
        return kInvalidArgument;
    sampleRate = s.sampleRate;
    engine.prepare(sampleRate, snapshot());
    return kResultOk;
}
tresult PLUGIN_API Processor::setProcessing(TBool v) {
    if (v) {
        engine.prepare(sampleRate, snapshot());
        meterIn.store(0);
        meterOut.store(0);
    }
    return kResultOk;
}

template <class Sample> tresult Processor::processTyped(ProcessData &d) {
    struct Queue {
        IParamValueQueue *queue = nullptr;
        int32 cursor = 0, offset = 0;
        ParamValue value = 0;
        bool valid = false;
    };
    std::array<Queue, Count> queues{};
    auto advance = [](Queue &q) {
        q.valid = q.queue && q.cursor < q.queue->getPointCount() &&
                  q.queue->getPoint(q.cursor++, q.offset, q.value) == kResultTrue;
    };
    engine.setAll(snapshot());
    if (clearRequested.exchange(false, std::memory_order_acq_rel)) engine.clear();
    if (d.processContext && (d.processContext->state & ProcessContext::kTempoValid))
        engine.setTempo(d.processContext->tempo);
    if (d.inputParameterChanges) {
        const int32 count = d.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < count; ++i) {
            auto q = d.inputParameterChanges->getParameterData(i);
            if (q && q->getParameterId() < Count) {
                auto &c = queues[q->getParameterId()];
                c.queue = q;
                advance(c);
            }
        }
    }
    auto apply = [&](unsigned id, double v) {
        v = canonical(id, v);
        parameters[id].store(v, std::memory_order_relaxed);
        engine.set(id, v);
    };
    if (d.numSamples == 0) {
        for (unsigned id = 0; id < Count; ++id) {
            auto &q = queues[id];
            while (q.valid) {
                apply(id, q.value);
                advance(q);
            }
        }
        return kResultOk;
    }
    AudioBusBuffers *in = d.numInputs > 0 ? d.inputs : nullptr;
    AudioBusBuffers *out = d.numOutputs > 0 ? d.outputs : nullptr;
    Sample **ib = nullptr;
    Sample **ob = nullptr;
    if constexpr (sizeof(Sample) == sizeof(float)) {
        if (in)
            ib = in->channelBuffers32;
        if (out)
            ob = out->channelBuffers32;
    } else {
        if (in)
            ib = in->channelBuffers64;
        if (out)
            ob = out->channelBuffers64;
    }
    double inPeak = 0, outPeak = 0;
    for (int32 i = 0; i < d.numSamples; ++i) {
        for (unsigned id = 0; id < Count; ++id) {
            auto &q = queues[id];
            while (q.valid && q.offset <= i) {
                apply(id, q.value);
                advance(q);
            }
        }
        double x[2] = {0, 0};
        for (int c = 0; c < 2; ++c) {
            const int source = inputChannels == 1 ? 0 : c;
            if (inputActive && in && ib && source < in->numChannels && ib[source] &&
                !(in->silenceFlags & (uint64(1) << source)))
                x[c] = ib[source][i];
            if (!std::isfinite(x[c]))
                x[c] = 0;
            inPeak = std::max(inPeak, std::abs(x[c]));
        }
        double stereo[2]{};
        engine.process(x[0], x[1], stereo[0], stereo[1]);
        for (int c = 0; c < outputChannels; ++c) {
            const double y = outputChannels == 1 ? (stereo[0] + stereo[1]) * .5 : stereo[c];
            if (outputActive && out && ob && c < out->numChannels && ob[c]) {
                ob[c][i] = static_cast<Sample>(y);
                outPeak = std::max(outPeak, std::abs(y));
            } else if (out && ob && c < out->numChannels && ob[c])
                ob[c][i] = 0;
        }
        // Safely clear any host-provided extra channels rather than leaving garbage.
        if (out && ob)
            for (int c = outputChannels; c < out->numChannels; ++c)
                if (ob[c])
                    ob[c][i] = 0;
    }
    if (out)
        out->silenceFlags =
            outPeak == 0
                ? (out->numChannels >= 64 ? ~uint64(0) : (uint64(1) << out->numChannels) - 1)
                : 0;
    const double decay = std::exp(-d.numSamples / (sampleRate * .18));
    meterIn.store(std::max(inPeak, meterIn.load(std::memory_order_relaxed) * decay),
                  std::memory_order_relaxed);
    meterOut.store(std::max(outPeak, meterOut.load(std::memory_order_relaxed) * decay),
                   std::memory_order_relaxed);
    return kResultOk;
}
tresult PLUGIN_API Processor::process(ProcessData &d) {
    if (d.numSamples < 0)
        return kInvalidArgument;
    if (d.symbolicSampleSize == kSample32)
        return processTyped<float>(d);
    if (d.symbolicSampleSize == kSample64)
        return processTyped<double>(d);
    return kResultFalse;
}
tresult PLUGIN_API Processor::connect(IConnectionPoint *p) {
    if (!p || peer)
        return kResultFalse;
    peer = p;
    peer->addRef();
    return kResultOk;
}
tresult PLUGIN_API Processor::disconnect(IConnectionPoint *p) {
    if (p != peer)
        return kResultFalse;
    if (peer) {
        peer->release();
        peer = nullptr;
    }
    return kResultOk;
}
tresult PLUGIN_API Processor::notify(IMessage *m) {
    if (!m || !m->getMessageID())
        return kInvalidArgument;
    if (!std::strcmp(m->getMessageID(), "Afterglow.ClearTails")) {
        clearRequested.store(true, std::memory_order_release); return kResultOk;
    }
    if (std::strcmp(m->getMessageID(), "Afterglow.RequestMeters") || !peer)
        return kResultFalse;
    auto response = new MeterMessage("Afterglow.Meters");
    response->setFloat("in", meterIn.load(std::memory_order_relaxed));
    response->setFloat("out", meterOut.load(std::memory_order_relaxed));
    peer->notify(response);
    response->release();
    return kResultOk;
}

Controller::~Controller() {
    if (handler)
        handler->release();
    if (peer)
        peer->release();
}
tresult PLUGIN_API Controller::queryInterface(const TUID id, void **out) {
    if (!out)
        return kInvalidArgument;
    *out = nullptr;
    if (sameID(id, FUnknown_iid) || sameID(id, IEditController_iid) || sameID(id, IPluginBase_iid))
        *out = static_cast<IEditController *>(this);
    else if (sameID(id, IConnectionPoint_iid))
        *out = static_cast<IConnectionPoint *>(this);
    if (!*out)
        return kNoInterface;
    addRef();
    return kResultOk;
}
tresult PLUGIN_API Controller::initialize(FUnknown *) {
    if (initialized)
        return kResultFalse;
    initialized = true;
    return kResultOk;
}
tresult PLUGIN_API Controller::terminate() {
    setComponentHandler(nullptr);
    if (peer) {
        auto p = peer;
        peer = nullptr;
        p->release();
    }
    initialized = false;
    return kResultOk;
}
tresult PLUGIN_API Controller::setComponentState(IBStream *s) {
    Values v;
    if (!readValues(s, v))
        return kResultFalse;
    values = v;
    return kResultOk;
}
tresult PLUGIN_API Controller::setState(IBStream *s) {
    uint32 v = 0;
    return readExact(s, &v, sizeof(v)) && v == 1 ? kResultOk : kResultFalse;
}
tresult PLUGIN_API Controller::getState(IBStream *s) {
    const uint32 v = 1;
    return writeExact(s, &v, sizeof(v)) ? kResultOk : kResultFalse;
}
tresult PLUGIN_API Controller::getParameterInfo(int32 i, ParameterInfo &p) {
    if (i < 0 || i >= Count)
        return kInvalidArgument;
    p = {};
    p.id = i;
    text16(p.title, definitions[i].name);
    text16(p.shortTitle, definitions[i].name);
    text16(p.units, definitions[i].unit);
    p.stepCount = definitions[i].steps;
    p.defaultNormalizedValue = definitions[i].initial;
    p.unitId = 0;
    p.flags = ParameterInfo::kCanAutomate;
    if (i == Bypass)
        p.flags |= ParameterInfo::kIsBypass;
    if (definitions[i].steps > 1 || i == Sync || i == Trails)
        p.flags |= ParameterInfo::kIsList;
    return kResultOk;
}
tresult PLUGIN_API Controller::getParamStringByValue(ParamID id, ParamValue v, String128 out) {
    if (id >= Count || !out)
        return kInvalidArgument;
    char text[128];
    format(id, v, text, sizeof(text));
    text16(out, text);
    return kResultOk;
}
tresult PLUGIN_API Controller::getParamValueByString(ParamID id, TChar *text, ParamValue &v) {
    if (id >= Count || !text)
        return kInvalidArgument;
    char s[128]{};
    for (int i = 0; i < 127 && text[i]; ++i)
        s[i] = text[i] < 128 ? char(text[i]) : '?';
    if (definitions[id].steps) {
        for (int i = 0; i <= definitions[id].steps; ++i) {
            char name[128];
            format(id, double(i) / definitions[id].steps, name, sizeof(name));
            bool eq = true;
            for (int j = 0; name[j] || s[j]; ++j) {
                if (std::tolower(static_cast<unsigned char>(name[j])) !=
                    std::tolower(static_cast<unsigned char>(s[j]))) {
                    eq = false;
                    break;
                }
            }
            if (eq) {
                v = double(i) / definitions[id].steps;
                return kResultOk;
            }
        }
    }
    char *end = nullptr;
    const double plain = std::strtod(s, &end);
    if (end == s || !std::isfinite(plain))
        return kResultFalse;
    v = fromPlain(id, plain);
    return kResultOk;
}
tresult PLUGIN_API Controller::setParamNormalized(ParamID id, ParamValue v) {
    if (id >= Count || !std::isfinite(v))
        return kInvalidArgument;
    values[id] = canonical(id, v);
    return kResultOk;
}
tresult PLUGIN_API Controller::setComponentHandler(IComponentHandler *h) {
    if (h)
        h->addRef();
    if (handler)
        handler->release();
    handler = h;
    return kResultOk;
}
IPlugView *PLUGIN_API Controller::createView(FIDString name) {
    return name && !std::strcmp(name, ViewType::kEditor) ? makeEditor(this) : nullptr;
}
void Controller::begin(unsigned id) {
    if (handler && id < Count)
        handler->beginEdit(id);
}
void Controller::edit(unsigned id, double v) {
    if (id >= Count)
        return;
    const double next = canonical(id, v);
    if (values[id] == next)
        return;
    values[id] = next;
    if (handler)
        handler->performEdit(id, values[id]);
}
void Controller::end(unsigned id) {
    if (handler && id < Count)
        handler->endEdit(id);
}
void Controller::loadPreset(int index) {
    if (index < 0 || index >= presetCount)
        return;
    for (unsigned i = 0; i < Count; ++i)
        if (i != Bypass)
            setFromUI(i, presets[index].values[i]);
}
int Controller::currentPreset() const {
    for (int p = 0; p < presetCount; ++p) {
        bool same = true;
        for (unsigned i = 0; i < Count; ++i)
            if (i != Bypass && i != Freeze && i != DelayOn && i != ReverbOn && std::abs(values[i] - presets[p].values[i]) > .0001)
                same = false;
        if (same)
            return p;
    }
    return -1;
}
tresult PLUGIN_API Controller::connect(IConnectionPoint *p) {
    if (!p || peer)
        return kResultFalse;
    peer = p;
    peer->addRef();
    return kResultOk;
}
tresult PLUGIN_API Controller::disconnect(IConnectionPoint *p) {
    if (p != peer)
        return kResultFalse;
    if (peer) {
        peer->release();
        peer = nullptr;
    }
    return kResultOk;
}
tresult PLUGIN_API Controller::notify(IMessage *m) {
    if (!m || !m->getMessageID() || std::strcmp(m->getMessageID(), "Afterglow.Meters"))
        return kResultFalse;
    auto a = m->getAttributes();
    if (!a)
        return kResultFalse;
    a->getFloat("in", meterInput);
    a->getFloat("out", meterOutput);
    return kResultOk;
}
void Controller::clearTails() {
    setFromUI(Freeze, 0);
    if (peer) { auto m = new MeterMessage("Afterglow.ClearTails"); peer->notify(m); m->release(); }
}
void Controller::pollMeters() {
    if (peer) {
        auto m = new MeterMessage("Afterglow.RequestMeters");
        peer->notify(m);
        m->release();
    } else {
        meterInput *= .8;
        meterOutput *= .8;
    }
}

class Factory final : public IPluginFactory3 {
  public:
    tresult PLUGIN_API queryInterface(const TUID id, void **out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (sameID(id, FUnknown_iid) || sameID(id, IPluginFactory_iid) ||
            sameID(id, IPluginFactory2_iid) || sameID(id, IPluginFactory3_iid)) {
            *out = static_cast<IPluginFactory3 *>(this);
            addRef();
            return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override {
        auto n = --refs;
        if (!n)
            delete this;
        return n;
    }
    tresult PLUGIN_API getFactoryInfo(PFactoryInfo *p) override {
        if (!p)
            return kInvalidArgument;
        *p = {};
        std::strcpy(p->vendor, kVendor);
        p->flags = PFactoryInfo::kUnicode;
        return kResultOk;
    }
    int32 PLUGIN_API countClasses() override { return 2; }
    tresult PLUGIN_API getClassInfo(int32 i, PClassInfo *p) override {
        if (!p || i < 0 || i > 1)
            return kInvalidArgument;
        *p = {};
        std::memcpy(p->cid, i ? controllerUID : processorUID, 16);
        p->cardinality = PClassInfo::kManyInstances;
        std::strcpy(p->category, i ? kVstComponentControllerClass : kVstAudioEffectClass);
        std::strcpy(p->name, i ? "Afterglow Controller" : "Afterglow");
        return kResultOk;
    }
    tresult PLUGIN_API getClassInfo2(int32 i, PClassInfo2 *p) override {
        if (!p || i < 0 || i > 1)
            return kInvalidArgument;
        *p = {};
        PClassInfo c;
        getClassInfo(i, &c);
        std::memcpy(p->cid, c.cid, 16);
        p->cardinality = c.cardinality;
        std::strcpy(p->category, c.category);
        std::strcpy(p->name, c.name);
        std::strcpy(p->subCategories, i ? "" : "Fx|Delay|Reverb");
        std::strcpy(p->vendor, kVendor);
        std::strcpy(p->version, kVersion);
        std::strcpy(p->sdkVersion, kVstVersionString);
        return kResultOk;
    }
    tresult PLUGIN_API getClassInfoUnicode(int32 i, PClassInfoW *p) override {
        if (!p)
            return kInvalidArgument;
        PClassInfo2 c;
        auto r = getClassInfo2(i, &c);
        if (r != kResultOk)
            return r;
        *p = {};
        std::memcpy(p->cid, c.cid, 16);
        p->cardinality = c.cardinality;
        std::strcpy(p->category, c.category);
        std::strcpy(p->subCategories, c.subCategories);
        text16(p->name, c.name, 64);
        text16(p->vendor, c.vendor, 64);
        text16(p->version, c.version, 64);
        text16(p->sdkVersion, c.sdkVersion, 64);
        return kResultOk;
    }
    tresult PLUGIN_API setHostContext(FUnknown *) override { return kResultOk; }
    tresult PLUGIN_API createInstance(FIDString cid, FIDString iid, void **out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (!cid || !iid)
            return kInvalidArgument;
        if (sameID(cid, processorUID)) {
            auto p = new Processor;
            auto r = p->queryInterface(iid, out);
            p->release();
            return r;
        }
        if (sameID(cid, controllerUID)) {
            auto c = new Controller;
            auto r = c->queryInterface(iid, out);
            c->release();
            return r;
        }
        return kNoInterface;
    }

  private:
    std::atomic<uint32> refs{1};
};
} // namespace afterglow

extern "C" SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory *PLUGIN_API GetPluginFactory() {
    return new afterglow::Factory;
}
#ifdef _WIN32
extern "C" __declspec(dllexport) bool InitDll() {
    return true;
}
extern "C" __declspec(dllexport) bool ExitDll() {
    return true;
}
#else
extern "C" __attribute__((visibility("default"))) bool ModuleEntry(void *) {
    return true;
}
extern "C" __attribute__((visibility("default"))) bool ModuleExit() {
    return true;
}
#endif
