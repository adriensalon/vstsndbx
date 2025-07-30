#pragma once

#include <public.sdk/source/vst/hosting/module.h>
#include <public.sdk/source/vst/hosting/plugprovider.h>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

#include <filesystem>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <vector>

struct sandboxed_plugin_instance {
    std::shared_ptr<Steinberg::Vst::PlugProvider> plugin_provider { nullptr };
    Steinberg::Vst::IComponent* instance_processor { nullptr };
    Steinberg::Vst::IEditController* instance_controller { nullptr };
    bool is_processor_created { false };
    bool is_controller_created { false };
};

struct sandboxed_plugin_data {
    std::filesystem::path plugin_path {};
    std::string plugin_name {};
    std::string plugin_version {};
    Steinberg::FUID original_processor_uid {};
    Steinberg::FUID original_controller_uid {};
    Steinberg::FUID proxy_processor_uid {};
    Steinberg::FUID proxy_controller_uid {};
    VST3::Hosting::ClassInfo class_info {};
    std::unordered_map<std::size_t, std::shared_ptr<sandboxed_plugin_instance>> instances {};
    std::unordered_map<std::size_t, std::shared_ptr<sandboxed_plugin_instance>> pending_instances {};
    std::mutex pending_mutex {};
    std::atomic<std::size_t> next_id { 0 };
    std::atomic<bool> should_increment { false };
};

struct sandboxed_proxy_data {
    sandboxed_plugin_data* plugin_data;
    std::size_t instance_id;
};

struct sandbox_processor : public Steinberg::Vst::AudioEffect {

    sandbox_processor(const sandboxed_proxy_data& proxy_data)
        : _proxy_data(proxy_data)
    {
        Steinberg::TUID _controller_uid = INLINE_UID_FROM_FUID(_proxy_data.plugin_data->proxy_controller_uid);
        setControllerClass(_controller_uid);
    }

    ~sandbox_processor() override
    {
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override
    {
        // _original_processor->initialize(nullptr);

        auto result = AudioEffect::initialize(context);
        if (result == Steinberg::kResultTrue) {
            addAudioInput(STR("Input"), Steinberg::Vst::SpeakerArr::kStereo);
            addAudioOutput(STR("Output"), Steinberg::Vst::SpeakerArr::kStereo);
        }

        // return _sandboxed_processor->initialize(context);
        // return Steinberg::kResultOk;

        return result;
    }

    Steinberg::tresult PLUGIN_API terminate() override
    {
        return AudioEffect::terminate();
    }

    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override
    {
        // Steinberg::tresult _result = _original_processor->setActive(state);
        return AudioEffect::setActive(state);
    }

    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) override
    {
        return AudioEffect::setupProcessing(newSetup);
    }

    Steinberg::tresult PLUGIN_API canProcessSampleSize(std::int32_t symbolicSampleSize) override
    {
        if (symbolicSampleSize == Steinberg::Vst::kSample32)
            return Steinberg::kResultTrue;

        // disable the following comment if your processing support kSample64
        /* if (symbolicSampleSize == Vst::kSample64)
            return kResultTrue; */

        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override
    {
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

    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override
    {
        // Steinberg::IBStreamer streamer (state, kLittleEndian);
        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override
    {
        // Steinberg::IBStreamer streamer (state, kLittleEndian);
        return Steinberg::kResultOk;
    }

private:
    sandboxed_proxy_data _proxy_data;
};

struct sandbox_controller : public Steinberg::Vst::EditControllerEx1 {

    sandbox_controller(const sandboxed_proxy_data& proxy_data)
        : _proxy_data(proxy_data)
    {
    }

    ~sandbox_controller() override
    {
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override
    {

        return EditControllerEx1::initialize(context);
    }

    Steinberg::tresult PLUGIN_API terminate() override
    {
        return EditControllerEx1::terminate();
    }

    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override
    {
        return Steinberg::kResultOk;
    }

    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override
    {
        return nullptr;
    }

    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override
    {
        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override
    {
        return Steinberg::kResultOk;
    }

private:
    sandboxed_proxy_data _proxy_data;
};
