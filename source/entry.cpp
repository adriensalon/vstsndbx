// vst3sdk
#include <public.sdk/source/main/pluginfactory.h>
#include <public.sdk/source/vst/hosting/module.h>
#include <public.sdk/source/vst/hosting/plugprovider.h>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

#include <sndbx.hpp>

// rapidjson
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

// std
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <sstream>

extern Steinberg::FUnknown* create_sndbx_effect_instance(void* context);
extern Steinberg::FUnknown* create_sndbx_controller_instance(void* context);

// proxy plugins constants
#define sndbx_vendor "sndbx"
#define sndbx_url "github.com/adriensalon/vstsndbx"
#define sndbx_email "adrien.salon@live.fr"

/// @brief Represents the original plugin data
struct original_plugin_data {
    std::filesystem::path plugin_path;
    Steinberg::Vst::IComponent* plugin_component;
    Steinberg::Vst::IEditController* plugin_controller;
    VST3::UID component_uid;
    VST3::UID controller_uid;
    std::string plugin_version;
};

/// @brief Represents the proxy plugin data
struct proxy_plugin_data {
    Steinberg::FUnknown* (*processor_create_instance_function)(void*);
    Steinberg::FUnknown* (*controller_create_instance_function)(void*);
};

/// @brief Loads all the original plugins paths from the specified json path
[[nodiscard]] std::vector<std::filesystem::path> load_json_plugin_paths(const std::filesystem::path& json_path)
{
    std::vector<std::filesystem::path> result;

    // Lire le fichier JSON
    std::ifstream file(json_path);
    if (!file) {
        std::cerr << "Erreur : impossible d'ouvrir " << json_path << std::endl;
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    // Parse avec RapidJSON
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(json.c_str());
    if (!ok) {
        std::cerr << "Erreur de parsing : " << rapidjson::GetParseError_En(ok.Code())
                  << " (offset " << ok.Offset() << ")" << std::endl;
        return result;
    }

    // Extraire les chemins
    if (!doc.HasMember("paths") || !doc["paths"].IsArray()) {
        std::cerr << "Champ 'paths' manquant ou invalide." << std::endl;
        return result;
    }

    for (const auto& val : doc["paths"].GetArray()) {
        if (val.IsString()) {
            result.emplace_back(std::filesystem::path(val.GetString()));
        }
    }

    return result;
}

/// @brief Loads all the original plugins contained in each original plugin path (multiple plugins in a module)
[[nodiscard]] std::vector<original_plugin_data> load_original_plugins(const std::filesystem::path& plugin_path)
{
    original_plugin_data _original_plugin {};
    std::string _error;
    VST3::Hosting::Module::Ptr _module = VST3::Hosting::Module::create(plugin_path.string(), _error);
    if (!_module) {
        std::string _reason = "Could not create Module for file:";
        _reason += plugin_path.string();
        _reason += "\nError: ";
        _reason += _error;
        // EditorHost::IPlatform::instance ().kill (-1, _reason);
        // return;
    }
    VST3::Hosting::PluginFactory factory = _module->getFactory();

    Steinberg::IPtr<Steinberg::Vst::PlugProvider> plugProvider;
    for (auto& classInfo : factory.classInfos()) {
        if (classInfo.category() == kVstAudioEffectClass) {

            plugProvider = Steinberg::owned(new Steinberg::Vst::PlugProvider(factory, classInfo, true));
            break;
        }
    }
    if (!plugProvider) {
        std::string error;
        error = "No VST3 Audio Module Class found in file ";
        error += plugin_path.string();
        // EditorHost::IPlatform::instance ().kill (-1, error);
        // return ;
    }

    // _original_plugin.component_uid = factory.classInfos()[0].ID(); CRASH
    // _original_plugin.controller_uid = factory.classInfos()[1].ID(); CRASH
    _original_plugin.plugin_component = plugProvider->getComponent();
    _original_plugin.plugin_controller = plugProvider->getController();
    _original_plugin.component_uid = ProcessorUID; // TODO
    _original_plugin.controller_uid = ControllerUID; // TODO
    _original_plugin.plugin_path = plugin_path;
    _original_plugin.plugin_version = "1.0.0"; // get from classes?
    auto midiMapping = Steinberg::U::cast<Steinberg::Vst::IMidiMapping>(_original_plugin.plugin_controller);

    //! TODO: Query the plugProvider for a proper name which gets displayed in JACK.
    // auto vst3Processor = AudioClient::create("VST 3 SDK", component, midiMapping);

    return { _original_plugin };
}

/// @brief Registers the proxy plugin to the plugin factory
void register_proxy_plugin(Steinberg::CPluginFactory* factory, const original_plugin_data& original_plugin, const proxy_plugin_data& create_info)
{
    const std::string _plugin_name = original_plugin.plugin_path.filename().replace_extension("").string();

    Steinberg::PClassInfo2 _processor_class(
        original_plugin.component_uid.data(),
        Steinberg::PClassInfo::kManyInstances,
        kVstAudioEffectClass,
        (_plugin_name + " (Sandboxed)").c_str(),
        Steinberg::Vst::kDistributable,
        "Fx",
        nullptr,
        original_plugin.plugin_version.c_str(),
        kVstVersionString);

    Steinberg::PClassInfo2 _controller_class(
        original_plugin.controller_uid.data(),
        Steinberg::PClassInfo::kManyInstances,
        kVstComponentControllerClass,
        (_plugin_name + " [Controller] (Sandboxed)").c_str(),
        0,
        "",
        nullptr,
        original_plugin.plugin_version.c_str(),
        kVstVersionString);

    factory->registerClass(&_processor_class, create_info.processor_create_instance_function, original_plugin.plugin_component);
    factory->registerClass(&_controller_class, create_info.controller_create_instance_function, original_plugin.plugin_controller);
}

/// @brief Exported function to create the proxy plugin from the host
SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory* PLUGIN_API GetPluginFactory()
{
    if (Steinberg::gPluginFactory) {
        Steinberg::gPluginFactory->addRef();

    } else {
        static Steinberg::PFactoryInfo _factory_info(
            sndbx_vendor,
            sndbx_url,
            sndbx_email,
            Steinberg::Vst::kDefaultFactoryFlags);

        // TODO FIND JSON NEAR sndbx dll
        std::filesystem::path _json_path = "C:/Users/adri/dev/vstsndbx/.dev/conf.json";

        Steinberg::gPluginFactory = new Steinberg::CPluginFactory(_factory_info);

        for (const std::filesystem::path& _plugin_path : load_json_plugin_paths(_json_path)) {
            for (const original_plugin_data& _original_plugin : load_original_plugins(_plugin_path)) {
                proxy_plugin_data _proxy_plugin = {
                    _proxy_plugin.processor_create_instance_function = create_sndbx_effect_instance,
                    _proxy_plugin.controller_create_instance_function = create_sndbx_controller_instance
                };
                register_proxy_plugin(Steinberg::gPluginFactory, _original_plugin, _proxy_plugin);
            }
        }
    }

    return Steinberg::gPluginFactory;
}
