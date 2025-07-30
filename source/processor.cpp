#include <sandbox.hpp>

sandbox_processor::sandbox_processor(const sandboxed_proxy_data& proxy_data)
    : _proxy_data(proxy_data)
{
    Steinberg::TUID _controller_uid = INLINE_UID_FROM_FUID(_proxy_data.plugin_data->proxy_controller_uid);
    setControllerClass(_controller_uid);
}

sandbox_processor::~sandbox_processor()
{
}

Steinberg::tresult PLUGIN_API sandbox_processor::initialize(Steinberg::FUnknown* context)
{

    auto result = AudioEffect::initialize(context);
    if (result == Steinberg::kResultTrue) {
        addAudioInput(STR("Input"), Steinberg::Vst::SpeakerArr::kStereo);
        addAudioOutput(STR("Output"), Steinberg::Vst::SpeakerArr::kStereo);
    }

    // return _sandboxed_processor->initialize(context);
    // return Steinberg::kResultOk;
    
    // sandboxed_plugin_instance* _instance = _proxy_data.plugin_data->instances[_proxy_data.instance_id].get();
    // _proxy_data.plugin_data->instances[_proxy_data.instance_id]->instance_processor->initialize(context);

    return result;
}

Steinberg::tresult PLUGIN_API sandbox_processor::terminate()
{
    return AudioEffect::terminate();
}

Steinberg::tresult PLUGIN_API sandbox_processor::setActive(Steinberg::TBool state)
{
    // Steinberg::tresult _result = _original_processor->setActive(state);
    return AudioEffect::setActive(state);
}

Steinberg::tresult PLUGIN_API sandbox_processor::setupProcessing(Steinberg::Vst::ProcessSetup& newSetup)
{
    return AudioEffect::setupProcessing(newSetup);
}

Steinberg::tresult PLUGIN_API sandbox_processor::canProcessSampleSize(std::int32_t symbolicSampleSize)
{
    if (symbolicSampleSize == Steinberg::Vst::kSample32)
        return Steinberg::kResultTrue;

    // disable the following comment if your processing support kSample64
    /* if (symbolicSampleSize == Vst::kSample64)
            return kResultTrue; */

    return Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API sandbox_processor::process(Steinberg::Vst::ProcessData& data)
{
    static bool ok = false;
    if (!ok) {

    }
    if (data.numSamples > 0) {
        //--- ------------------------------------------
        // here as example a default implementation where we try to copy the inputs to the outputs:
        // if less input than outputs then clear outputs
        //--- ------------------------------------------

        std::int32_t minBus = std::min(data.numInputs, data.numOutputs);
        for (std::int32_t i = 0; i < minBus; i++) {
            std::int32_t minChan = std::min(data.inputs[i].numChannels, data.outputs[i].numChannels);
            for (std::int32_t c = 0; c < minChan; c++) {
                // do not need to be copied if the buffers are the same
                if (data.outputs[i].channelBuffers32[c] != data.inputs[i].channelBuffers32[c]) {
                    std::memcpy(data.outputs[i].channelBuffers32[c], data.inputs[i].channelBuffers32[c],
                        data.numSamples * sizeof(Steinberg::Vst::Sample32));
                }
            }
            data.outputs[i].silenceFlags = data.inputs[i].silenceFlags;

            // clear the remaining output buffers
            for (std::int32_t c = minChan; c < data.outputs[i].numChannels; c++) {
                // clear output buffers
                std::memset(data.outputs[i].channelBuffers32[c], 0,
                    data.numSamples * sizeof(Steinberg::Vst::Sample32));

                // inform the host that this channel is silent
                data.outputs[i].silenceFlags |= ((std::uint64_t)1 << c);
            }
        }
        // clear the remaining output buffers
        for (std::int32_t i = minBus; i < data.numOutputs; i++) {
            // clear output buffers
            for (std::int32_t c = 0; c < data.outputs[i].numChannels; c++) {
                memset(data.outputs[i].channelBuffers32[c], 0,
                    data.numSamples * sizeof(Steinberg::Vst::Sample32));
            }
            // inform the host that this bus is silent
            data.outputs[i].silenceFlags = ((std::uint64_t)1 << data.outputs[i].numChannels) - 1;
        }
    }
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API sandbox_processor::setState(Steinberg::IBStream* state)
{
    // Steinberg::IBStreamer streamer (state, kLittleEndian);
    _proxy_data.plugin_data->instances[_proxy_data.instance_id]->instance_processor->setState(state);
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API sandbox_processor::getState(Steinberg::IBStream* state)
{
    // Steinberg::IBStreamer streamer (state, kLittleEndian);
    _proxy_data.plugin_data->instances[_proxy_data.instance_id]->instance_processor->getState(state);
    return Steinberg::kResultOk;
}