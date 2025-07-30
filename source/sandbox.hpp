#pragma once

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/vst/hosting/plugprovider.h>

#include <filesystem>
#include <vector>

struct sandboxed_plugin_data {
    std::filesystem::path plugin_path;
    std::string plugin_name;
    std::unique_ptr<Steinberg::Vst::PlugProvider> plugin_provider;
    Steinberg::Vst::IComponent* plugin_processor; // ................................. cannot use smart ptrs bc abstract interface
    Steinberg::Vst::IEditController* plugin_controller; // ........................... cannot use smart ptrs bc abstract interface
    Steinberg::FUID original_processor_uid;
    Steinberg::FUID original_controller_uid;
    Steinberg::FUID proxy_processor_uid;
    Steinberg::FUID proxy_controller_uid;
    std::string plugin_version;
};
