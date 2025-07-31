#include <sandbox.hpp>

std::vector<Steinberg::Vst::BusInfo> get_bus_infos(Steinberg::Vst::IComponent* component)
{
    std::vector<Steinberg::Vst::BusInfo> _bus_infos;

    for (Steinberg::int32 dir = 0; dir < 2; ++dir) { // 0 = input, 1 = output
        Steinberg::Vst::MediaType mediaTypes[] = {
            Steinberg::Vst::kAudio,
            Steinberg::Vst::kEvent
        };

        for (Steinberg::Vst::MediaType mediaType : mediaTypes) {
            Steinberg::int32 busCount = component->getBusCount(mediaType, dir);
            for (Steinberg::int32 i = 0; i < busCount; ++i) {
                Steinberg::Vst::BusInfo busInfo {};
                if (component->getBusInfo(mediaType, dir, i, busInfo) == Steinberg::kResultOk) {
                    _bus_infos.push_back(busInfo); // you'll need a struct to store direction+mediaType+info
                }
            }
        }
    }
    return _bus_infos;
}

sandbox_processor::sandbox_processor(const sandboxed_proxy_data& proxy_data)
    : _proxy_data(proxy_data)
{
    Steinberg::TUID _controller_uid = INLINE_UID_FROM_FUID(_proxy_data.plugin_data->proxy_controller_uid);
    setControllerClass(_controller_uid);
}

sandbox_processor::~sandbox_processor()
{
}

Steinberg::Vst::SpeakerArrangement mapChannelCountToArrangement(Steinberg::uint32 channelCount)
{
    switch (channelCount) {
    case 1:
        return Steinberg::Vst::SpeakerArr::kMono;
    case 2:
        return Steinberg::Vst::SpeakerArr::kStereo;
    default:
        return Steinberg::Vst::SpeakerArr::kEmpty; // Or throw / assert
    }
}

Steinberg::tresult PLUGIN_API sandbox_processor::initialize(Steinberg::FUnknown* context)
{

    if (AudioEffect::initialize(context) != Steinberg::kResultOk) {
        std::cout << "Sandbox error: kResultFalse from base proxy class AudioEffect::initialize" << std::endl;
        return Steinberg::kResultFalse;
    }

    std::vector<Steinberg::Vst::BusInfo> _bus_infos = get_bus_infos(_proxy_data.get_sandboxed_processor());

    for (const Steinberg::Vst::BusInfo& bus : _bus_infos) {
        if (bus.mediaType == Steinberg::Vst::kAudio) {
            if (bus.mediaType == Steinberg::Vst::kAudio) {
                Steinberg::Vst::SpeakerArrangement arr = mapChannelCountToArrangement(bus.channelCount);
                Steinberg::int32 expectedChannels = Steinberg::Vst::SpeakerArr::getChannelCount(arr);

                if (expectedChannels != (Steinberg::int32)bus.channelCount) {
                    std::cerr << "Channel mismatch on bus: " << bus.name
                              << " | plugin says: " << bus.channelCount
                              << ", arrangement says: " << expectedChannels << std::endl;
                    continue; // or return kResultFalse if you want to fail hard
                }

                if (bus.direction == Steinberg::Vst::kInput)
                    addAudioInput(bus.name, arr, bus.busType, bus.flags);
                else
                    addAudioOutput(bus.name, arr, bus.busType, bus.flags);
            }

        } else if (bus.mediaType == Steinberg::Vst::kEvent) {
            if (bus.direction == Steinberg::Vst::kInput)
                addEventInput(bus.name, bus.channelCount, bus.busType, bus.flags);
            else
                addEventOutput(bus.name, bus.channelCount, bus.busType, bus.flags);
        }
    }

    _proxy_data.get_sandboxed_processor()->initialize(context);

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API sandbox_processor::terminate()
{
    // _proxy_data.get_sandboxed_processor()->terminate();
    // no we must call ourself plugprovider + module shutdown bc on a pas d'autre entry point pour ca
    // + delete instance de la map
    return AudioEffect::terminate();
}

Steinberg::tresult PLUGIN_API sandbox_processor::setActive(Steinberg::TBool state)
{
    if (_proxy_data.get_sandboxed_processor()->setActive(state) != Steinberg::kResultOk) {
        std::cout << "Sandbox error: kResultFalse from sandboxed class IAudioProcessor::setActive" << std::endl;
        return Steinberg::kResultFalse;
    }

    if (AudioEffect::setActive(state) != Steinberg::kResultOk) {
        std::cout << "Sandbox error: kResultFalse from base proxy class AudioEffect::setActive" << std::endl;
        return Steinberg::kResultFalse;
    }

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API sandbox_processor::setupProcessing(Steinberg::Vst::ProcessSetup& newSetup)
{

    Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> _audio_processor(_proxy_data.get_sandboxed_processor());
    if (!_audio_processor) {
        std::cout << "Sandbox error: interface IAudioProcessor not implemented by sandboxed class" << std::endl;
        return Steinberg::kResultFalse;
    }
    if (_audio_processor->setupProcessing(newSetup) != Steinberg::kResultOk) {
        std::cout << "Sandbox error: kResultFalse from sandboxed class IAudioProcessor::setupProcessing" << std::endl;
        return Steinberg::kResultFalse;
    }

    if (AudioEffect::setupProcessing(newSetup) != Steinberg::kResultOk) {
        std::cout << "Sandbox error: kResultFalse from base proxy class AudioEffect::setupProcessing" << std::endl;
        return Steinberg::kResultFalse;
    }

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API sandbox_processor::canProcessSampleSize(std::int32_t symbolicSampleSize)
{
    Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> _audio_processor(_proxy_data.get_sandboxed_processor());
    if (!_audio_processor) {
        std::cout << "Sandbox error: interface IAudioProcessor not implemented by sandboxed class" << std::endl;
        return Steinberg::kResultFalse;
    }

    return _audio_processor->canProcessSampleSize(symbolicSampleSize);
}

Steinberg::tresult PLUGIN_API sandbox_processor::process(Steinberg::Vst::ProcessData& data)
{
    Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> _audio_processor(_proxy_data.get_sandboxed_processor());
    if (!_audio_processor) {
        std::cout << "Sandbox error: interface IAudioProcessor not implemented by sandboxed class" << std::endl;
        return Steinberg::kResultFalse;
    }
    if (_audio_processor->process(data) != Steinberg::kResultOk) {
        std::cout << "Sandbox error: kResultFalse from sandboxed class IAudioProcessor::process" << std::endl;
        return Steinberg::kResultFalse;
    }

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API sandbox_processor::setState(Steinberg::IBStream* state)
{
    // Steinberg::IBStreamer streamer (state, kLittleEndian);    later we save state of the sandboxed plugin too
    return _proxy_data.get_sandboxed_processor()->setState(state);
}

Steinberg::tresult PLUGIN_API sandbox_processor::getState(Steinberg::IBStream* state)
{
    // Steinberg::IBStreamer streamer (state, kLittleEndian);        later we save state of the sandboxed plugin too
    return _proxy_data.get_sandboxed_processor()->getState(state);
}

Steinberg::tresult PLUGIN_API sandbox_processor::setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts)
{
    if (auto sandboxed = Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor>(_proxy_data.get_sandboxed_processor())) {
        return sandboxed->setBusArrangements(inputs, numIns, outputs, numOuts);
    }
    return Steinberg::kResultFalse; // Or log error
}

Steinberg::tresult PLUGIN_API sandbox_processor::getBusArrangement(Steinberg::Vst::BusDirection dir, Steinberg::int32 busIndex, Steinberg::Vst::SpeakerArrangement& arr)
{
    if (auto sandboxed = Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor>(_proxy_data.get_sandboxed_processor())) {
        return sandboxed->getBusArrangement(dir, busIndex, arr);
    }
    return Steinberg::kResultFalse;
}