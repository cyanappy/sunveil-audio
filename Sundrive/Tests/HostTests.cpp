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
using namespace sundrive;
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
    int begins = 0, ends = 0;
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

static void audioTests(IPluginFactory *factory) {
    Instance p(factory);
    check(p.controller->getParameterCount() == Count, "all nine controls exposed to host");
    for (unsigned id = 0; id < Count; ++id) {
        ParameterInfo info{};
        check(p.controller->getParameterInfo(id, info) == kResultOk && info.id == id,
              "stable host parameter metadata");
        check(info.flags & ParameterInfo::kCanAutomate, "all controls automate");
        String128 text{};
        ParamValue parsed = 0;
        p.controller->getParamStringByValue(id, definitions[id].initial, text);
        check(p.controller->getParamValueByString(id, text, parsed) == kResultOk &&
                  std::abs(parsed - definitions[id].initial) < .006,
              "host parameter text roundtrip");
    }
    ParameterInfo bypassInfo;
    p.controller->getParameterInfo(Bypass, bypassInfo);
    check(bypassInfo.flags & ParameterInfo::kIsBypass, "host identifies bypass correctly");
    check(p.audio->getLatencySamples() == 5, "fixed low 5-sample latency");
    std::vector<float> left(1024), right(1024), outL(1024), outR(1024);
    auto v = defaults();
    v[Bypass] = 1;
    p.set(v);
    p.configure();
    left[0] = 1;
    runAudio(p, 1024, left.data(), right.data(), outL.data(), outR.data());
    for (int i = 0; i < 1024; ++i) {
        check(std::abs(outL[i] - (i == Engine::latency ? 1.f : 0.f)) < 1e-7,
              "bypass exactly preserves delayed input");
        check(outR[i] == 0, "stereo channels remain independent");
    }
    p.configure(48000, kSample32, 1, 2);
    std::fill(left.begin(), left.end(), .125f);
    runAudio<float>(p, 1024, left.data(), nullptr, left.data(), outR.data(), nullptr, 1, 2);
    for (int i = Engine::latency; i < 1024; ++i)
        check(left[i] == outR[i], "in-place mono-to-stereo does not reread overwritten input");
    p.configure(48000, kSample32, 2, 2);
    v = defaults();
    v[Bypass] = 1;
    p.set(v);
    p.configure();
    std::fill(left.begin(), left.end(), .2f);
    std::fill(right.begin(), right.end(), 0.f);
    Changes automation;
    automation.add(Bypass, 256, 0);
    automation.add(Bypass, 768, 1);
    runAudio(p, 1024, left.data(), right.data(), outL.data(), outR.data(), &automation);
    for (int i = Engine::latency; i < 256; ++i)
        check(outL[i] == .2f, "automation does not apply before its sample offset");
    check(std::abs(outL[600] - .2f) > 1e-5, "automation changes the processed audio");
    for (double sr : {44100., 48000., 96000., 192000.}) {
        p.configure(sr, kSample32);
        for (int voice = 0; voice < 2; ++voice)
            for (int clip = 0; clip < 3; ++clip) {
                v = defaults();
                v[Drive] = .9;
                v[Voice] = voice;
                v[Clipping] = clip / 2.0;
                p.set(v);
                for (int block = 0; block < 16; ++block) {
                    for (int i = 0; i < 1024; ++i) {
                        left[i] = float(.4 * std::sin(2 * pi * 997 * (block * 1024 + i) / sr));
                        right[i] = 0;
                    }
                    runAudio(p, 1024, left.data(), right.data(), outL.data(), outR.data());
                    for (int i = 0; i < 1024; ++i)
                        check(std::isfinite(outL[i]) && std::isfinite(outR[i]),
                              "all voicings, clippers, and sample rates produce finite audio");
                }
            }
    }
    p.configure(48000, kSample64);
    std::vector<double> a(1024, .1), b(1024, 0), c(1024), d(1024);
    runAudio(p, 1024, a.data(), b.data(), c.data(), d.data());
    check(std::isfinite(c[100]), "64-bit audio processing");
    // Hostile input must not poison the filter state or create an endless NaN tail.
    a[10] = std::numeric_limits<double>::quiet_NaN();
    a[20] = std::numeric_limits<double>::infinity();
    runAudio(p, 1024, a.data(), b.data(), c.data(), d.data());
    for (auto x : c)
        check(std::isfinite(x), "NaN and infinity inputs are sanitized");
    v = presets[4].values;
    p.set(v);
    Stream saved;
    check(p.component->getState(&saved) == kResultOk, "save processor state");
    {
        Instance restored(factory);
        saved.pos = 0;
        check(restored.component->setState(&saved) == kResultOk, "restore processor state");
        Stream second;
        restored.component->getState(&second);
        check(second.bytes == saved.bytes, "saved state roundtrips byte for byte");
        saved.pos = 0;
        restored.controller->setComponentState(&saved);
        for (unsigned i = 0; i < Count; ++i)
            check(restored.controller->getParamNormalized(i) == v[i],
                  "controller restores all saved knobs and switches");
        Stream broken = saved;
        broken.pos = 0;
        broken.bytes[0] = 0;
        check(restored.component->setState(&broken) != kResultOk, "corrupt state rejected");
        Stream after;
        restored.component->getState(&after);
        check(after.bytes == second.bytes, "bad state cannot overwrite good settings");
    }
    std::puts("PASS: factory, parameters, bus layouts, latency, automation, precision, robustness, "
              "and state.");
}

static void dspTests() {
    Oversampler os;
    double max = 0;
    int at = -1;
    std::array<double, 2048> impulse{};
    double sum = 0, moment = 0;
    for (int i = 0; i < static_cast<int>(impulse.size()); ++i) {
        const double y = os.tick(i == 0 ? 1. : 0., [](double x) { return x; });
        impulse[i] = y;
        sum += y;
        moment += i * y;
        if (std::abs(y) > max) {
            max = std::abs(y);
            at = i;
        }
    }
    // Minimum-phase delay varies with frequency: its broadband impulse peak
    // need not equal the low-frequency delay used for host compensation.
    check(at == 6, "minimum-phase transient peak is reduced to six samples");
    check(std::abs(moment / sum - 5) < 1.e-6,
          "measured low-frequency group delay matches host compensation");
    check(std::abs(sum - 1) < 1.e-7, "complete oversampling path has unity DC gain");
    // Independent full convolution of the v1.0 prototype. The new filter changes
    // phase, while its anti-alias magnitude must remain essentially unchanged.
    std::array<double, 129> original{};
    double originalSum = 0;
    for (int i = 0; i < 129; ++i) {
        const double t = i - 64.0;
        const double sinc = t == 0 ? .225 : std::sin(2 * pi * .1125 * t) / (pi * t);
        original[i] =
            sinc * (.42 - .5 * std::cos(2 * pi * i / 128) + .08 * std::cos(4 * pi * i / 128));
        originalSum += original[i];
    }
    for (auto &h : original)
        h /= originalSum;
    std::array<double, 65> reference{};
    for (int i = 0; i < 129; ++i)
        for (int j = 0; j < 129; ++j)
            if ((i + j) % 4 == 0)
                reference[(i + j) / 4] += 4 * original[i] * original[j];
    double worstDifference = 0;
    for (int bin = 0; bin <= 128; ++bin) {
        const double w = .8 * pi * bin / 128;
        std::complex<double> before{}, after{};
        for (size_t i = 0; i < reference.size(); ++i)
            before += reference[i] * std::polar(1.0, -w * i);
        for (size_t i = 0; i < impulse.size(); ++i)
            after += impulse[i] * std::polar(1.0, -w * i);
        const double difference = std::abs(20 * std::log10(std::abs(after) / std::abs(before)));
        worstDifference = std::max(worstDifference, difference);
        check(difference < .001,
              "low-latency oversampling retains the original passband magnitude");
    }
    std::printf(
        "Low-latency impulse peak: %d samples; DC delay: %.9f; magnitude difference: %.9g dB.\n",
        at, moment / sum, worstDifference);
    os.reset();
    check(os.tick(0., [](double x) { return x; }) == 0,
          "reset clears fractional-alignment filter state");
    double dc = 0;
    for (int i = 0; i < 1000; ++i)
        dc = os.tick(1., [](double x) { return x; });
    check(std::abs(dc - 1) < 1.e-5, "oversampling preserves DC gain");
    Biquad eq;
    eq.peak(48000, 750, .75, 12);
    double x = 0, y = 0;
    for (int i = 0; i < 48000; ++i) {
        double a = std::sin(2 * pi * 750 * i / 48000);
        double b = eq.tick(a);
        if (i > 24000) {
            x += a * a;
            y += b * b;
        }
    }
    check(std::abs(10 * std::log10(y / x) - 12) < .03,
          "mid EQ delivers specified gain at its center frequency");
    std::puts("PASS: oversampling alignment, unity gain, and EQ response.");
}

#ifdef _WIN32
static LRESULT CALLBACK hostProc(HWND w, UINT m, WPARAM a, LPARAM b) {
    return DefWindowProcW(w, m, a, b);
}
struct Frame final : IPlugFrame {
    HWND parent;
    IPlugView *view;
    tresult PLUGIN_API queryInterface(const TUID i, void **p) override {
        *p = nullptr;
        if (sameID(i, FUnknown_iid) || sameID(i, IPlugFrame_iid)) {
            *p = this;
            return kResultOk;
        }
        return kNoInterface;
    }
    HOST_REFS
    tresult PLUGIN_API resizeView(IPlugView *v, ViewRect *r) override {
        RECT b{0, 0, r->getWidth(), r->getHeight()};
        AdjustWindowRect(&b, WS_OVERLAPPEDWINDOW, FALSE);
        SetWindowPos(parent, nullptr, 0, 0, b.right - b.left, b.bottom - b.top,
                     SWP_NOMOVE | SWP_NOZORDER);
        return v->onSize(r);
    }
};
static void pump() {
    MSG m;
    for (int i = 0; i < 12; ++i) {
        while (PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
        Sleep(10);
    }
}
static void screenshot(HWND child, const char *filename) {
    RECT r;
    GetClientRect(child, &r);
    const int w = r.right, h = r.bottom;
    HDC src = GetDC(child), dc = CreateCompatibleDC(src);
    auto bitmap = CreateCompatibleBitmap(src, w, h);
    auto old = SelectObject(dc, bitmap);
    BitBlt(dc, 0, 0, w, h, src, 0, 0, SRCCOPY);
    SelectObject(dc, old);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = w;
    info.bmiHeader.biHeight = -h;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    std::vector<unsigned char> pixels(w * h * 4);
    GetDIBits(dc, bitmap, 0, h, pixels.data(), &info, DIB_RGB_COLORS);
    BITMAPFILEHEADER header{};
    header.bfType = 0x4D42;
    header.bfOffBits = sizeof(header) + sizeof(info.bmiHeader);
    header.bfSize = header.bfOffBits + DWORD(pixels.size());
    FILE *f = std::fopen(filename, "wb");
    check(f != nullptr, "write native editor screenshot");
    std::fwrite(&header, sizeof(header), 1, f);
    std::fwrite(&info.bmiHeader, sizeof(info.bmiHeader), 1, f);
    std::fwrite(pixels.data(), pixels.size(), 1, f);
    std::fclose(f);
    DeleteObject(bitmap);
    DeleteDC(dc);
    ReleaseDC(child, src);
}
static void uiTests(IPluginFactory *factory) {
    Instance p(factory);
    p.configure();
    auto view = p.controller->createView(ViewType::kEditor);
    check(view != nullptr, "create native Windows editor");
    WNDCLASSW wc{};
    wc.lpfnWndProc = hostProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"SundriveTestHost";
    RegisterClassW(&wc);
    RECT r{0, 0, artwork::W, artwork::H};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    HWND parent =
        CreateWindowW(wc.lpszClassName, L"Sundrive Test Host", WS_OVERLAPPEDWINDOW, 50, 50,
                      r.right - r.left, r.bottom - r.top, nullptr, nullptr, wc.hInstance, nullptr);
    Frame frame;
    frame.parent = parent;
    frame.view = view;
    view->setFrame(&frame);
    check(view->attached(parent, kPlatformTypeHWND) == kResultOk,
          "attach native editor to host window");
    ShowWindow(parent, SW_SHOW);
    pump();
    HWND child = FindWindowExW(parent, nullptr, nullptr, L"Sundrive");
    check(child != nullptr, "native child editor created");
    SetFocus(child);
    auto click = [child](int x, int y) {
        SendMessageW(child, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
        SendMessageW(child, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    };
    const auto &voice = artwork::voiceBox;
    const auto &clipping = artwork::clipBox;
    const auto &drive = artwork::knobs[0];
    click(voice.x + voice.w * 3 / 4, voice.y + voice.h / 2);
    check(p.controller->getParamNormalized(Voice) == 1, "voice switch updates controller");
    click(clipping.x + clipping.w / 2, clipping.y + clipping.h / 2);
    check(p.controller->getParamNormalized(Clipping) == .5, "clipping toggle selects LED");
    click(clipping.x + clipping.w / 2, clipping.y + clipping.h / 2);
    check(p.controller->getParamNormalized(Clipping) == 1, "clipping toggle selects Asym");
    click(clipping.x + clipping.w / 2, clipping.y + clipping.h / 2);
    check(p.controller->getParamNormalized(Clipping) == 0, "clipping toggle cycles to Soft");
    click(voice.x + voice.w / 2, voice.y + voice.h / 2);
    check(p.controller->getParamNormalized(Voice) == 0, "voice toggle returns to Transparent");
    SendMessageW(child, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(drive.x, drive.y));
    SendMessageW(child, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(drive.x, drive.y - 45));
    SendMessageW(child, WM_LBUTTONUP, 0, MAKELPARAM(drive.x, drive.y - 45));
    check(p.controller->getParamNormalized(Drive) > .4, "drive knob drag updates its value");
    SendMessageW(child, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(drive.x, drive.y));
    SendMessageW(child, WM_LBUTTONUP, 0, MAKELPARAM(drive.x, drive.y));
    SendMessageW(child, WM_LBUTTONDBLCLK, MK_LBUTTON, MAKELPARAM(drive.x, drive.y));
    SendMessageW(child, WM_LBUTTONUP, 0, MAKELPARAM(drive.x, drive.y));
    check(p.controller->getParamNormalized(Drive) == definitions[Drive].initial,
          "double click resets knob");
    click(artwork::stompX, artwork::stompY);
    check(p.controller->getParamNormalized(Bypass) == 1, "footswitch bypasses pedal");
    SendMessageW(child, WM_KEYDOWN, VK_SPACE, 0);
    check(p.controller->getParamNormalized(Bypass) == 0, "space key engages pedal");
    check(view->onKeyDown(' ', KEY_SPACE, 0) == kResultTrue &&
              p.controller->getParamNormalized(Bypass) == 1,
          "host-forwarded VST3 key codes toggle bypass");
    view->onKeyDown(' ', KEY_SPACE, 0);
    check(p.handler.begins == p.handler.ends, "all UI gestures are balanced");
    std::vector<float> left(512, .1f), right(512, 0), outL(512), outR(512);
    runAudio(p, 512, left.data(), right.data(), outL.data(), outR.data(), &p.handler.pending);
    p.handler.pending.clear();
    // Return to the initial preset for a useful screenshot, through host automation.
    Changes reset;
    for (unsigned i = 0; i < Count; ++i) {
        p.controller->setParamNormalized(i, definitions[i].initial);
        reset.add(i, 0, definitions[i].initial);
    }
    runAudio(p, 512, left.data(), right.data(), outL.data(), outR.data(), &reset);
    for (int i = 0; i < 40; ++i) {
        for (int n = 0; n < 512; ++n)
            left[n] = float(.12 * std::sin(2 * pi * 220 * (i * 512 + n) / 48000));
        runAudio(p, 512, left.data(), right.data(), outL.data(), outR.data());
    }
    pump();
    screenshot(child, "Sundrive-editor.bmp");
    IPlugViewContentScaleSupport *scale = nullptr;
    check(view->queryInterface(IPlugViewContentScaleSupport_iid,
                               reinterpret_cast<void **>(&scale)) == kResultOk,
          "DPI scale interface");
    check(scale->setContentScaleFactor(1.5f) == kResultTrue, "150 percent scaling");
    pump();
    ViewRect size;
    view->getSize(&size);
    check(size.getWidth() == int(artwork::W * 1.5) && size.getHeight() == int(artwork::H * 1.5),
          "editor resizes proportionally at high DPI");
    screenshot(child, "Sundrive-editor-150.bmp");
    scale->release();
    view->removed();
    view->setFrame(nullptr);
    view->release();
    DestroyWindow(parent);
    std::puts("PASS: native editor, controls, host edit gestures, bypass, and high DPI.");
}
#endif

int main(int argc, char **argv) {
    check(argc >= 2, "plugin module path supplied");
#ifdef _WIN32
    auto module = LoadLibraryA(argv[1]);
    if (!module)
        std::fprintf(stderr, "LoadLibrary error: %lu\n", GetLastError());
    check(module != nullptr, "load Windows VST3 module");
    auto init = reinterpret_cast<bool (*)()>(GetProcAddress(module, "InitDll"));
    auto exit = reinterpret_cast<bool (*)()>(GetProcAddress(module, "ExitDll"));
    check(init && exit && init(), "Windows module entry points");
    auto factoryProc = reinterpret_cast<GetFactoryProc>(GetProcAddress(module, "GetPluginFactory"));
#else
    auto module = dlopen(argv[1], RTLD_NOW);
    if (!module)
        std::fprintf(stderr, "%s\n", dlerror());
    check(module != nullptr, "load native VST3 test module");
    auto factoryProc = reinterpret_cast<GetFactoryProc>(dlsym(module, "GetPluginFactory"));
#endif
    check(factoryProc != nullptr, "VST3 factory export");
    auto factory = factoryProc();
    check(factory && factory->countClasses() == 2,
          "separate audio processor and controller factories");
    const auto begin = std::chrono::steady_clock::now();
    dspTests();
    audioTests(factory);
#ifdef _WIN32
    uiTests(factory);
#endif
    factory->release();
#ifdef _WIN32
    exit();
    FreeLibrary(module);
#else
    dlclose(module);
#endif
    const double elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
    std::printf("ALL PASS: %d checks in %.3f seconds.\n", checks, elapsed);
    return 0;
}
