#pragma once
#include "VstSupport.h"

namespace afterglow {

// These messages are constructed and dispatched only on the editor/UI thread.
// The audio thread publishes meter envelopes through lock-free atomics.
class MeterMessage final : public IMessage, public IAttributeList {
  public:
    explicit MeterMessage(const char *id) : messageID(id) {}
    tresult PLUGIN_API queryInterface(const TUID id, void **out) override {
        if (!out)
            return kInvalidArgument;
        *out = nullptr;
        if (sameID(id, FUnknown_iid) || sameID(id, IMessage_iid))
            *out = static_cast<IMessage *>(this);
        else if (sameID(id, IAttributeList_iid))
            *out = static_cast<IAttributeList *>(this);
        if (!*out)
            return kNoInterface;
        addRef();
        return kResultOk;
    }
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override {
        auto n = --refs;
        if (!n)
            delete this;
        return n;
    }
    FIDString PLUGIN_API getMessageID() override { return messageID; }
    void PLUGIN_API setMessageID(FIDString id) override { messageID = id; }
    IAttributeList *PLUGIN_API getAttributes() override { return this; }
    tresult PLUGIN_API setFloat(AttrID id, double value) override {
        if (!id)
            return kInvalidArgument;
        if (!std::strcmp(id, "in"))
            in = value;
        else if (!std::strcmp(id, "out"))
            out = value;
        else
            return kResultFalse;
        return kResultOk;
    }
    tresult PLUGIN_API getFloat(AttrID id, double &value) override {
        if (!id)
            return kInvalidArgument;
        if (!std::strcmp(id, "in"))
            value = in;
        else if (!std::strcmp(id, "out"))
            value = out;
        else
            return kResultFalse;
        return kResultOk;
    }
    tresult PLUGIN_API setInt(AttrID, int64) override { return kNotImplemented; }
    tresult PLUGIN_API getInt(AttrID, int64 &) override { return kNotImplemented; }
    tresult PLUGIN_API setString(AttrID, const TChar *) override { return kNotImplemented; }
    tresult PLUGIN_API getString(AttrID, TChar *, uint32) override { return kNotImplemented; }
    tresult PLUGIN_API setBinary(AttrID, const void *, uint32) override { return kNotImplemented; }
    tresult PLUGIN_API getBinary(AttrID, const void *&, uint32 &) override {
        return kNotImplemented;
    }

  private:
    std::atomic<uint32> refs{1};
    const char *messageID;
    double in = 0, out = 0;
};

class Processor final : public IComponent,
                        public IAudioProcessor,
                        public IConnectionPoint,
                        public IProcessContextRequirements {
  public:
    Processor();
    ~Processor();
    tresult PLUGIN_API queryInterface(const TUID, void **) override;
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override {
        auto n = --refs;
        if (!n)
            delete this;
        return n;
    }
    tresult PLUGIN_API initialize(FUnknown *) override;
    tresult PLUGIN_API terminate() override;
    tresult PLUGIN_API getControllerClassId(TUID) override;
    tresult PLUGIN_API setIoMode(IoMode) override { return kResultOk; }
    int32 PLUGIN_API getBusCount(MediaType, BusDirection) override;
    tresult PLUGIN_API getBusInfo(MediaType, BusDirection, int32, BusInfo &) override;
    tresult PLUGIN_API getRoutingInfo(RoutingInfo &, RoutingInfo &) override {
        return kNotImplemented;
    }
    tresult PLUGIN_API activateBus(MediaType, BusDirection, int32, TBool) override;
    tresult PLUGIN_API setActive(TBool) override;
    tresult PLUGIN_API setState(IBStream *) override;
    tresult PLUGIN_API getState(IBStream *) override;
    tresult PLUGIN_API setBusArrangements(SpeakerArrangement *, int32, SpeakerArrangement *,
                                          int32) override;
    tresult PLUGIN_API getBusArrangement(BusDirection, int32, SpeakerArrangement &) override;
    tresult PLUGIN_API canProcessSampleSize(int32 s) override {
        return s == kSample32 || s == kSample64 ? kResultTrue : kResultFalse;
    }
    uint32 PLUGIN_API getLatencySamples() override { return Engine::latency; }
    tresult PLUGIN_API setupProcessing(ProcessSetup &) override;
    tresult PLUGIN_API setProcessing(TBool) override;
    tresult PLUGIN_API process(ProcessData &) override;
    uint32 PLUGIN_API getTailSamples() override {
        return kInfiniteTail; // Freeze can sustain indefinitely.
    }
    uint32 PLUGIN_API getProcessContextRequirements() override { return IProcessContextRequirements::kNeedTempo; }
    tresult PLUGIN_API connect(IConnectionPoint *) override;
    tresult PLUGIN_API disconnect(IConnectionPoint *) override;
    tresult PLUGIN_API notify(IMessage *) override;

  private:
    template <typename Sample> tresult processTyped(ProcessData &);
    Values snapshot() const;
    std::atomic<uint32> refs{1};
    std::array<std::atomic<double>, Count> parameters;
    std::atomic<double> meterIn{0}, meterOut{0};
    std::atomic<bool> clearRequested{false};
    Engine engine;
    double sampleRate = 48000;
    int inputChannels = 2, outputChannels = 2;
    bool initialized = false, inputActive = true, outputActive = true;
    IConnectionPoint *peer = nullptr;
};

class Controller final : public IEditController, public IConnectionPoint {
  public:
    Controller() = default;
    ~Controller();
    tresult PLUGIN_API queryInterface(const TUID, void **) override;
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override {
        auto n = --refs;
        if (!n)
            delete this;
        return n;
    }
    tresult PLUGIN_API initialize(FUnknown *) override;
    tresult PLUGIN_API terminate() override;
    tresult PLUGIN_API setComponentState(IBStream *) override;
    tresult PLUGIN_API setState(IBStream *) override;
    tresult PLUGIN_API getState(IBStream *) override;
    int32 PLUGIN_API getParameterCount() override { return Count; }
    tresult PLUGIN_API getParameterInfo(int32, ParameterInfo &) override;
    tresult PLUGIN_API getParamStringByValue(ParamID, ParamValue, String128) override;
    tresult PLUGIN_API getParamValueByString(ParamID, TChar *, ParamValue &) override;
    ParamValue PLUGIN_API normalizedParamToPlain(ParamID id, ParamValue v) override {
        return id < Count ? toPlain(id, v) : 0;
    }
    ParamValue PLUGIN_API plainParamToNormalized(ParamID id, ParamValue v) override {
        return id < Count ? fromPlain(id, v) : 0;
    }
    ParamValue PLUGIN_API getParamNormalized(ParamID id) override {
        return id < Count ? values[id] : 0;
    }
    tresult PLUGIN_API setParamNormalized(ParamID, ParamValue) override;
    tresult PLUGIN_API setComponentHandler(IComponentHandler *) override;
    IPlugView *PLUGIN_API createView(FIDString) override;
    tresult PLUGIN_API connect(IConnectionPoint *) override;
    tresult PLUGIN_API disconnect(IConnectionPoint *) override;
    tresult PLUGIN_API notify(IMessage *) override;
    void begin(unsigned id);
    void edit(unsigned id, double value);
    void end(unsigned id);
    void setFromUI(unsigned id, double value) {
        if (id >= Count || canonical(id, value) == values[id])
            return;
        begin(id);
        edit(id, value);
        end(id);
    }
    void loadPreset(int index);
    int currentPreset() const;
    void pollMeters();
    void clearTails();
    double inputPeak() const { return meterInput; }
    double outputPeak() const { return meterOutput; }

  private:
    std::atomic<uint32> refs{1};
    Values values = defaults();
    IComponentHandler *handler = nullptr;
    IConnectionPoint *peer = nullptr;
    double meterInput = 0, meterOutput = 0;
    bool initialized = false;
};

IPlugView *makeEditor(Controller *controller);
} // namespace afterglow
