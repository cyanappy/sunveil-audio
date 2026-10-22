// A small real VST3 host harness. Loads the compiled module through its public
// factory and exercises the same COM-style interfaces used by a DAW.
#include "../Source/VstSupport.h"
#include "../Source/Artwork.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <random>
#include <chrono>
#include <complex>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "pluginterfaces/base/keycodes.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#else
#include <dlfcn.h>
#endif
using namespace afterglow;
static int checks = 0;
static void check(bool ok, const char *what) {
    ++checks;
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        std::exit(1);
    }
}
#define HOST_REFS                                                                                  \
    uint32 PLUGIN_API addRef() override {                                                          \
        return 1;                                                                                  \
    }                                                                                              \
    uint32 PLUGIN_API release() override {                                                         \
        return 1;                                                                                  \
    }

struct Stream final : IBStream {
    std::vector<unsigned char> bytes;
    size_t pos = 0;
    tresult PLUGIN_API queryInterface(const TUID i, void **p) override {
        *p = nullptr;
        if (sameID(i, FUnknown_iid) || sameID(i, IBStream_iid)) {
            *p = this;
            return kResultOk;
        }
        return kNoInterface;
    }
    HOST_REFS
    tresult PLUGIN_API read(void *p, int32 n, int32 *got) override {
        if (n < 0)
            return kInvalidArgument;
        const size_t len = std::min(size_t(n), bytes.size() - std::min(pos, bytes.size()));
        if (len)
            std::memcpy(p, bytes.data() + pos, len);
        pos += len;
        if (got)
            *got = int32(len);
        return len == size_t(n) ? kResultOk : kResultFalse;
    }
    tresult PLUGIN_API write(void *p, int32 n, int32 *put) override {
        if (n < 0)
            return kInvalidArgument;
        if (pos + n > bytes.size())
            bytes.resize(pos + n);
        std::memcpy(bytes.data() + pos, p, n);
        pos += n;
        if (put)
            *put = n;
        return kResultOk;
    }
    tresult PLUGIN_API seek(int64 n, int32 mode, int64 *result) override {
        const int64 target = (mode == kIBSeekCur   ? int64(pos)
                              : mode == kIBSeekEnd ? int64(bytes.size())
                                                   : 0) +
                             n;
        if (target < 0)
            return kInvalidArgument;
        pos = size_t(target);
        if (result)
            *result = pos;
        return kResultOk;
    }
    tresult PLUGIN_API tell(int64 *p) override {
        if (!p)
            return kInvalidArgument;
        *p = pos;
        return kResultOk;
    }
};
struct Point {
    int32 offset;
    double value;
};
struct Queue final : IParamValueQueue {
    unsigned id = 0;
    std::vector<Point> points;
    tresult PLUGIN_API queryInterface(const TUID i, void **p) override {
        *p = nullptr;
        if (sameID(i, FUnknown_iid) || sameID(i, IParamValueQueue_iid)) {
            *p = this;
            return kResultOk;
        }
        return kNoInterface;
    }
    HOST_REFS
    ParamID PLUGIN_API getParameterId() override { return id; }
    int32 PLUGIN_API getPointCount() override { return int32(points.size()); }
    tresult PLUGIN_API getPoint(int32 i, int32 &offset, ParamValue &v) override {
        if (i < 0 || i >= int32(points.size()))
            return kResultFalse;
        offset = points[i].offset;
        v = points[i].value;
        return kResultOk;
    }
    tresult PLUGIN_API addPoint(int32 offset, ParamValue v, int32 &index) override {
        index = int32(points.size());
        points.push_back({offset, v});
        return kResultOk;
    }
};
struct Changes final : IParameterChanges {
    std::array<Queue, Count> queues;
    std::vector<unsigned> active;
    Changes() {
        for (unsigned i = 0; i < Count; ++i)
            queues[i].id = i;
    }
    void clear() {
        active.clear();
        for (auto &q : queues)
            q.points.clear();
    }
    void add(unsigned id, int offset, double v) {
        int32 index;
        if (id >= Count)
            return;
        auto *q = addParameterData(id, index);
        q->addPoint(offset, v, index);
    }
    tresult PLUGIN_API queryInterface(const TUID i, void **p) override {
        *p = nullptr;
        if (sameID(i, FUnknown_iid) || sameID(i, IParameterChanges_iid)) {
            *p = this;
            return kResultOk;
        }
        return kNoInterface;
    }
    HOST_REFS
    int32 PLUGIN_API getParameterCount() override { return int32(active.size()); }
    IParamValueQueue *PLUGIN_API getParameterData(int32 i) override {
        return i >= 0 && i < int32(active.size()) ? &queues[active[i]] : nullptr;
    }
    IParamValueQueue *PLUGIN_API addParameterData(const ParamID &id, int32 &index) override {
        if (id >= Count)
            return nullptr;
        auto p = std::find(active.begin(), active.end(), id);
        if (p == active.end()) {
            active.push_back(id);
            index = int32(active.size() - 1);
        } else
            index = int32(p - active.begin());
        return &queues[id];
    }
};
struct Handler final : IComponentHandler {
    Changes pending;
    int begins = 0, ends = 0, edits = 0;
    std::array<int, Count> open{};
    tresult PLUGIN_API queryInterface(const TUID i, void **p) override {
        *p = nullptr;
        if (sameID(i, FUnknown_iid) || sameID(i, IComponentHandler_iid)) {
            *p = this;
            return kResultOk;
        }
        return kNoInterface;
    }
    HOST_REFS
    tresult PLUGIN_API beginEdit(ParamID id) override {
        check(id < Count, "GUI beginEdit parameter ID");
        check(open[id] == 0, "GUI gesture is not nested");
        open[id]++;
        begins++;
        return kResultOk;
    }
    tresult PLUGIN_API performEdit(ParamID id, ParamValue v) override {
        check(id < Count && open[id] == 1, "GUI automation has an open gesture");
        ++edits;
        pending.add(id, 0, v);
        return kResultOk;
    }
    tresult PLUGIN_API endEdit(ParamID id) override {
        check(id < Count && open[id] == 1, "GUI gesture closes exactly once");
        open[id]--;
        ends++;
        return kResultOk;
    }
    tresult PLUGIN_API restartComponent(int32) override { return kResultOk; }
};
struct HostContext final : FUnknown {
    tresult PLUGIN_API queryInterface(const TUID i, void **p) override {
        *p = nullptr;
        if (sameID(i, FUnknown_iid)) {
            *p = this;
            return kResultOk;
        }
        return kNoInterface;
    }
    HOST_REFS
};

struct Instance {
    IComponent *component = nullptr;
    IAudioProcessor *audio = nullptr;
    IEditController *controller = nullptr;
    IConnectionPoint *processorPeer = nullptr;
    IConnectionPoint *controllerPeer = nullptr;
    Handler handler;
    HostContext context;
    explicit Instance(IPluginFactory *factory) {
        check(factory->createInstance(processorUID, IComponent_iid,
                                      reinterpret_cast<void **>(&component)) == kResultOk,
              "create audio component");
        check(component->initialize(&context) == kResultOk, "initialize audio component");
        check(component->queryInterface(IAudioProcessor_iid, reinterpret_cast<void **>(&audio)) ==
                  kResultOk,
              "query audio interface");
        TUID cid;
        check(component->getControllerClassId(cid) == kResultOk, "get separate controller class");
        check(factory->createInstance(cid, IEditController_iid,
                                      reinterpret_cast<void **>(&controller)) == kResultOk,
              "create controller");
        check(controller->initialize(&context) == kResultOk, "initialize controller");
        controller->setComponentHandler(&handler);
        Stream state;
        component->getState(&state);
        state.pos = 0;
        check(controller->setComponentState(&state) == kResultOk,
              "synchronize controller from processor state");
        component->queryInterface(IConnectionPoint_iid, reinterpret_cast<void **>(&processorPeer));
        controller->queryInterface(IConnectionPoint_iid,
                                   reinterpret_cast<void **>(&controllerPeer));
        check(processorPeer && controllerPeer, "meter connection interfaces");
        processorPeer->connect(controllerPeer);
        controllerPeer->connect(processorPeer);
    }
    ~Instance() {
        component->setActive(false);
        audio->setProcessing(false);
        processorPeer->disconnect(controllerPeer);
        controllerPeer->disconnect(processorPeer);
        processorPeer->release();
        controllerPeer->release();
        controller->terminate();
        component->terminate();
        controller->release();
        audio->release();
        component->release();
    }
    void configure(double sr = 48000, int sampleSize = kSample32, int in = 2, int out = 2) {
        SpeakerArrangement ia = in == 1 ? SpeakerArr::kMono : SpeakerArr::kStereo,
                           oa = out == 1 ? SpeakerArr::kMono : SpeakerArr::kStereo;
        check(audio->setBusArrangements(&ia, 1, &oa, 1) == kResultOk,
              "set supported bus arrangements");
        ProcessSetup setup{kRealtime, sampleSize, 1024, sr};
        check(audio->setupProcessing(setup) == kResultOk, "configure sample rate and precision");
        component->activateBus(kAudio, kInput, 0, true);
        component->activateBus(kAudio, kOutput, 0, true);
        component->setActive(true);
        audio->setProcessing(true);
    }
    void set(const Values &v) {
        Changes change;
        for (unsigned i = 0; i < Count; ++i)
            change.add(i, 0, v[i]);
        ProcessData d;
        d.numSamples = 0;
        d.inputParameterChanges = &change;
        check(audio->process(d) == kResultOk, "flush parameters without audio buffers");
    }
};

template <class Sample>
static void runAudio(Instance &p, int count, Sample *in0, Sample *in1, Sample *out0, Sample *out1,
                     Changes *changes = nullptr, int ic = 2, int oc = 2) {
    Sample *ins[] = {in0, in1};
    Sample *outs[] = {out0, out1};
    AudioBusBuffers input, output;
    input.numChannels = ic;
    output.numChannels = oc;
    if constexpr (sizeof(Sample) == 4) {
        input.channelBuffers32 = ins;
        output.channelBuffers32 = outs;
    } else {
        input.channelBuffers64 = ins;
        output.channelBuffers64 = outs;
    }
    ProcessData d;
    d.symbolicSampleSize = sizeof(Sample) == 4 ? kSample32 : kSample64;
    d.numSamples = count;
    d.numInputs = d.numOutputs = 1;
    d.inputs = &input;
    d.outputs = &output;
    d.inputParameterChanges = changes;
    check(p.audio->process(d) == kResultOk, "audio process block succeeds");
}


#include "TestCases.inc"
