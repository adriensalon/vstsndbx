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
    std::shared_ptr<VST3::Hosting::Module> host_module;
    std::shared_ptr<Steinberg::Vst::PlugProvider> plugin_provider;
    Steinberg::Vst::IComponent* sandboxed_processor { nullptr }; // owned by plugin_provider
    Steinberg::Vst::IEditController* sandboxed_controller { nullptr }; // owned by plugin_provider
    bool is_proxy_processor_created { false }; // represents proxy!! should come first because we reorder
    bool is_proxy_controller_created { false }; // represents proxy!!
};

struct sandboxed_plugin_data {
    std::filesystem::path plugin_path;
    std::string plugin_name;
    std::string plugin_version;
    Steinberg::FUID original_processor_uid;
    Steinberg::FUID original_controller_uid;
    Steinberg::FUID proxy_processor_uid;
    Steinberg::FUID proxy_controller_uid;
    VST3::Hosting::ClassInfo class_info;
    std::vector<Steinberg::Vst::ParameterInfo> original_parameters;
    std::vector<Steinberg::Vst::UnitInfo> original_units;
    std::unordered_map<std::size_t, std::shared_ptr<sandboxed_plugin_instance>> sandboxed_instances;
    std::mutex pending_mutex;
    std::atomic<std::size_t> next_instance_id { 0 };
    std::atomic<bool> should_increment { false };
};

struct sandboxed_proxy_data {
    sandboxed_plugin_data* plugin_data; // owned by global static
    std::size_t instance_id;

    Steinberg::Vst::IComponent* get_sandboxed_processor();
    Steinberg::Vst::IEditController* get_sandboxed_controller();
};

struct sandbox_processor : public Steinberg::Vst::AudioEffect {
    sandbox_processor(const sandboxed_proxy_data& proxy_data);
    ~sandbox_processor() override;

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API terminate() override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(std::int32_t symbolicSampleSize) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;
    Steinberg::tresult PLUGIN_API getBusArrangement(Steinberg::Vst::BusDirection dir, Steinberg::int32 busIndex, Steinberg::Vst::SpeakerArrangement& arr) override;

private:
    sandboxed_proxy_data _proxy_data;
};

struct sandbox_controller : public Steinberg::Vst::EditControllerEx1 {
    sandbox_controller(const sandboxed_proxy_data& proxy_data);
    ~sandbox_controller() override;

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API terminate() override;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) override;
    Steinberg::Vst::ParamValue PLUGIN_API getParamNormalized(Steinberg::Vst::ParamID id) override;

    Steinberg::tresult beginEdit(Steinberg::Vst::ParamID tag) override; ///< to be called before a serie of performEdit
    Steinberg::tresult performEdit(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized) override; ///< will inform the host about the value change
    Steinberg::tresult endEdit(Steinberg::Vst::ParamID tag) override; ///< to be called after a serie of performEdit
    Steinberg::tresult startGroupEdit() override; ///< calls IComponentHandler2::startGroupEdit() if host supports it
    Steinberg::tresult finishGroupEdit() override; ///< calls IComponentHandler2::finishGroupEdit() if host supports it

private:
    sandboxed_proxy_data _proxy_data;
};
