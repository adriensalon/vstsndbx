#include <public.sdk/source/main/pluginfactory.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

#include <sandbox.hpp>

#define sandbox_vendor "sandbox"
#define sandbox_url "github.com/adriensalon/vstsandbox"
#define sandbox_email "adrien.salon@live.fr"
#define vstsandbox_json_file "vstsandbox.json"

#define VST_MAX_PATH 2048
extern Steinberg::tchar gPath[VST_MAX_PATH];

extern Steinberg::FUnknown* create_sandbox_processor_instance(void* context);
extern Steinberg::FUnknown* create_sandbox_controller_instance(void* context);

static std::vector<std::shared_ptr<sandboxed_plugin_data>> global_sandboxed_plugins;

[[nodiscard]] std::filesystem::path get_proxy_working_directory()
{
#if SMTG_OS_WINDOWS
    return std::filesystem::path(gPath).parent_path().parent_path().parent_path().parent_path();
#endif
    return std::filesystem::path(gPath).parent_path();
}

[[nodiscard]] std::string hash_to_uid_string(const std::string& input)
{
    std::hash<std::string> hasher;
    size_t h = hasher(input);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 4; ++i) {
        oss << std::setw(8) << static_cast<uint32_t>((h >> (i * 16)) & 0xFFFFFFFF);
    }

    std::string result = oss.str();
    result.resize(32, '0'); // Ensure it's 32 chars
    return result;
}

[[nodiscard]] Steinberg::FUID derive_proxy_uid(const Steinberg::FUID& original, const std::string& purpose)
{
    char fuid_str[33] = {};
    original.toString(fuid_str);

    std::string combined = std::string(fuid_str) + purpose;
    std::string hashed_str = hash_to_uid_string(combined);

    Steinberg::FUID proxy;
    proxy.fromString(hashed_str.c_str());
    return proxy;
}

[[nodiscard]] std::vector<std::filesystem::path> load_json_plugin_paths(const std::filesystem::path& json_path)
{
    std::vector<std::filesystem::path> result;
    rapidjson::Document doc;

    if (!std::filesystem::exists(json_path)) {
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("vst2_paths", rapidjson::Value(rapidjson::kArrayType), allocator);
        doc.AddMember("vst3_paths", rapidjson::Value(rapidjson::kArrayType), allocator);

        std::ofstream out(json_path);
        if (out) {
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);
            out << buffer.GetString();
        } else {
            std::cout << "Erreur : impossible de créer " << json_path << std::endl;
        }
    }

    // Lire le fichier JSON
    std::ifstream file(json_path);
    if (!file) {
        std::cout << "Erreur : impossible d'ouvrir " << json_path << std::endl;
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    // Parse avec RapidJSON
    rapidjson::ParseResult ok = doc.Parse(json.c_str());
    if (!ok) {
        std::cout << "Erreur de parsing : " << rapidjson::GetParseError_En(ok.Code())
                  << " (offset " << ok.Offset() << ")" << std::endl;
        return result;
    }

    // Extraire les chemins
    if (!doc.HasMember("vst3_paths") || !doc["vst3_paths"].IsArray()) {
        std::cout << "Champ 'vst3_paths' manquant ou invalide." << std::endl;
        return result;
    }

    for (const auto& val : doc["vst3_paths"].GetArray()) {
        if (val.IsString()) {
            result.emplace_back(std::filesystem::path(val.GetString()));
        }
    }

    return result;
}

[[nodiscard]] void load_sandboxed_plugins(const std::filesystem::path& plugin_path)
{
    std::string _module_error;
    VST3::Hosting::Module::Ptr _module = VST3::Hosting::Module::create(plugin_path.string(), _module_error);
    if (!_module) {
        std::cout << "Sandbox error: could not create Module from " << plugin_path << " with error " + _module_error << std::endl;
        return;
    }

    VST3::Hosting::PluginFactory _factory = _module->getFactory();
    for (VST3::Hosting::ClassInfo& _class_info : _factory.classInfos()) {

        if (_class_info.category() == kVstAudioEffectClass) {
            Steinberg::Vst::PlugProvider* _plugin_provider = new Steinberg::Vst::PlugProvider(_factory, _class_info, true);
            Steinberg::Vst::IComponent* _instance_processor = _plugin_provider->getComponent();
            Steinberg::Vst::IEditController* _instance_controller = _plugin_provider->getController();

            if (!_instance_processor) {
                // throw
            }

            if (!_instance_controller) {
                continue;
            }

            Steinberg::TUID _original_controller_tuid;
            if (_instance_processor->getControllerClassId(_original_controller_tuid) != Steinberg::kResultOk) {
                // normalement ca passe
            }

            std::shared_ptr<sandboxed_plugin_data> _original_plugin = global_sandboxed_plugins.emplace_back(std::make_shared<sandboxed_plugin_data>());

            _original_plugin->class_info = _class_info;

            _original_plugin->original_processor_uid = Steinberg::FUID::fromTUID(_class_info.ID().data());
            _original_plugin->original_controller_uid = Steinberg::FUID::fromTUID(_original_controller_tuid);

            static std::size_t _salt_id = 0;
            _original_plugin->proxy_processor_uid = derive_proxy_uid(_original_plugin->original_processor_uid, "p1" + std::to_string(_salt_id));
            _original_plugin->proxy_controller_uid = derive_proxy_uid(_original_plugin->original_controller_uid, "p2" + std::to_string(_salt_id));
            _salt_id++;

            _original_plugin->plugin_path = plugin_path;
            _original_plugin->plugin_name = _class_info.name();
            _original_plugin->plugin_version = _class_info.version();

            // auto midiMapping = Steinberg::U::cast<Steinberg::Vst::IMidiMapping>(_original_plugin.plugin_controller);

            std::size_t _parameters_count = _instance_controller->getParameterCount();
            _original_plugin->original_parameters.resize(_parameters_count);
            for (Steinberg::int32 _k = 0; _k < _parameters_count; ++_k) {
                Steinberg::Vst::ParameterInfo info;
                if (_instance_controller->getParameterInfo(_k, info) == Steinberg::kResultOk) {
                    _original_plugin->original_parameters[_k] = info;
                }
            }

            Steinberg::FUnknownPtr<Steinberg::Vst::IUnitInfo> _unit_info_interface(_instance_controller);
            if (_unit_info_interface) {
                std::size_t _units_count = _unit_info_interface->getUnitCount();
                _original_plugin->original_units.resize(_units_count);
                for (Steinberg::int32 i = 0; i < _units_count; ++i) {
                    Steinberg::Vst::UnitInfo info {};
                    if (_unit_info_interface->getUnitInfo(i, info) == Steinberg::kResultOk) {
                        _original_plugin->original_units.push_back(info);
                    }
                }
            }

            // Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> _audio_processor(_instance_processor);
            // Steinberg::Vst::SpeakerArrangement stereo = Steinberg::Vst::SpeakerArr::kMono;
            // _audio_processor->setBusArrangements(&stereo, 1, &stereo, 1);

            // for (Steinberg::int32 dir = 0; dir < 2; ++dir) { // 0 = input, 1 = output
            //     Steinberg::Vst::MediaType mediaTypes[] = {
            //         Steinberg::Vst::kAudio,
            //         Steinberg::Vst::kEvent
            //     };

            //     for (auto mediaType : mediaTypes) {
            //         Steinberg::int32 busCount = _instance_processor->getBusCount(mediaType, dir);
            //         for (Steinberg::int32 i = 0; i < busCount; ++i) {
            //             Steinberg::Vst::BusInfo busInfo {};
            //             if (_instance_processor->getBusInfo(mediaType, dir, i, busInfo) == Steinberg::kResultOk) {
            //                 _original_plugin->original_buses.push_back(busInfo); // you'll need a struct to store direction+mediaType+info
            //             }
            //         }
            //     }
            // }

            delete _plugin_provider;
        }
    }
}

struct proxy_plugin_callbacks {
    Steinberg::FUnknown* (*processor_create_instance_function)(void*);
    Steinberg::FUnknown* (*controller_create_instance_function)(void*);
};

void register_proxy_plugin(Steinberg::CPluginFactory* factory, sandboxed_plugin_data& original_plugin, const proxy_plugin_callbacks& create_info)
{
    Steinberg::TUID _proxy_processor_tuid = INLINE_UID_FROM_FUID(original_plugin.proxy_processor_uid);
    Steinberg::TUID _proxy_controller_tuid = INLINE_UID_FROM_FUID(original_plugin.proxy_controller_uid);

    Steinberg::PClassInfo2 _processor_class(
        _proxy_processor_tuid,
        Steinberg::PClassInfo::kManyInstances,
        kVstAudioEffectClass,
        (original_plugin.plugin_name + " (Sandboxed)").c_str(),
        Steinberg::Vst::kDistributable,
        "Fx", // TODO instruments !
        nullptr,
        original_plugin.plugin_version.c_str(),
        kVstVersionString);

    Steinberg::PClassInfo2 _controller_class(
        _proxy_controller_tuid,
        Steinberg::PClassInfo::kManyInstances,
        kVstComponentControllerClass,
        (original_plugin.plugin_name + " [Controller] (Sandboxed)").c_str(),
        0,
        "",
        nullptr,
        original_plugin.plugin_version.c_str(),
        kVstVersionString);

    factory->registerClass(&_processor_class, create_info.processor_create_instance_function, &original_plugin);
    factory->registerClass(&_controller_class, create_info.controller_create_instance_function, &original_plugin);
}

SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory* PLUGIN_API GetPluginFactory()
{
    if (Steinberg::gPluginFactory) {
        Steinberg::gPluginFactory->addRef();

    } else {
        static Steinberg::PFactoryInfo _factory_info(
            sandbox_vendor,
            sandbox_url,
            sandbox_email,
            Steinberg::Vst::kDefaultFactoryFlags);

        const std::filesystem::path _json_path = get_proxy_working_directory() / vstsandbox_json_file;

        Steinberg::gPluginFactory = new Steinberg::CPluginFactory(_factory_info);

        for (const std::filesystem::path& _plugin_path : load_json_plugin_paths(_json_path)) {
            load_sandboxed_plugins(_plugin_path);
        }

        for (std::shared_ptr<sandboxed_plugin_data>& _sandboxed_plugin : global_sandboxed_plugins) {
            proxy_plugin_callbacks _proxy_plugin;
            _proxy_plugin.processor_create_instance_function = create_sandbox_processor_instance;
            _proxy_plugin.controller_create_instance_function = create_sandbox_controller_instance;
            register_proxy_plugin(Steinberg::gPluginFactory, *(_sandboxed_plugin.get()), _proxy_plugin);
        }
    }

    return Steinberg::gPluginFactory;
}
