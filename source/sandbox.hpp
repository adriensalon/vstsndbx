#pragma once

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

#include <filesystem>

struct sandboxed_plugin_data {
    std::filesystem::path plugin_path;
    std::string plugin_name;
    Steinberg::Vst::IComponent* plugin_component;
    Steinberg::Vst::IEditController* plugin_controller;
    Steinberg::FUID original_processor_uid;
    Steinberg::FUID original_controller_uid;
    Steinberg::FUID proxy_processor_uid;
    Steinberg::FUID proxy_controller_uid;
    std::string plugin_version;
};