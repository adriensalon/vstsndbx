#include <sandbox.hpp>

std::shared_ptr<sandboxed_plugin_instance> get_or_create_pending_instance(sandboxed_plugin_data* data, std::size_t& id)
{
    std::lock_guard _lock(data->pending_mutex);

    if (data->should_increment.exchange(false)) {
        id = data->next_id.fetch_add(1);
    } else {
        id = data->next_id.load();
    }

    std::shared_ptr<sandboxed_plugin_instance>& _instance_slot = data->pending_instances[id];
    if (!_instance_slot) {
        _instance_slot = std::make_shared<sandboxed_plugin_instance>();
        _instance_slot->is_processor_created = false;
        _instance_slot->is_controller_created = false;
    }

    return _instance_slot;
}

void finalize_if_ready(sandboxed_plugin_data* data, const std::size_t id, const std::shared_ptr<sandboxed_plugin_instance>& inst)
{
    if (inst->is_processor_created && inst->is_controller_created) {
        std::lock_guard lock(data->pending_mutex);
        data->instances[id] = inst;
        data->pending_instances.erase(id);
        data->should_increment.store(true);
    }
}

Steinberg::FUnknown* create_sandbox_processor_instance(void* context)
{
    sandboxed_proxy_data _proxy_data;

    _proxy_data.plugin_data = static_cast<sandboxed_plugin_data*>(context);
    if (!_proxy_data.plugin_data) {
        const std::string _error_text = "Sandbox error: invalid context for sandbox processor creation";
        std::cerr << _error_text << std::endl;
        throw std::runtime_error(_error_text);
    }

    std::shared_ptr<sandboxed_plugin_instance> _plugin_instance = get_or_create_pending_instance(_proxy_data.plugin_data, _proxy_data.instance_id);

    std::string _module_error;
    VST3::Hosting::Module::Ptr _module = VST3::Hosting::Module::create(_proxy_data.plugin_data->plugin_path.string(), _module_error);
    if (!_module) {
        std::string _error_text = "Sandbox error: could not create Module with error " + _module_error;
        std::cerr << _error_text << std::endl;
        throw std::runtime_error(_error_text);
    }

    std::shared_ptr<Steinberg::Vst::PlugProvider> _plugin_provider = std::make_shared<Steinberg::Vst::PlugProvider>(_module->getFactory(), _proxy_data.plugin_data->class_info, true);
    _plugin_instance->plugin_provider = _plugin_provider;
    _plugin_instance->instance_processor = _plugin_provider->getComponent();
    _plugin_instance->instance_controller = _plugin_provider->getController();
    if (!_plugin_instance->instance_processor)
        return nullptr;

    _plugin_instance->is_processor_created = true;
    finalize_if_ready(_proxy_data.plugin_data, _proxy_data.instance_id, _plugin_instance);
    // _plugin_instance->instance_processor->addRef(); // Must addRef before returning
    
    return (Steinberg::Vst::IAudioProcessor*)new sandbox_processor(_proxy_data);
}

Steinberg::FUnknown* create_sandbox_controller_instance(void* context)
{
    sandboxed_proxy_data _proxy_data;

    _proxy_data.plugin_data = static_cast<sandboxed_plugin_data*>(context);
    if (!_proxy_data.plugin_data) {
        const std::string _error_text = "Sandbox error: invalid context for sandbox processor creation";
        std::cerr << _error_text << std::endl;
        throw std::runtime_error(_error_text);
    }

    std::shared_ptr<sandboxed_plugin_instance> _plugin_instance = get_or_create_pending_instance(_proxy_data.plugin_data, _proxy_data.instance_id);

    _plugin_instance->is_controller_created = true;
    finalize_if_ready(_proxy_data.plugin_data, _proxy_data.instance_id, _plugin_instance);
    // _plugin_instance->instance_controller->addRef(); // Must addRef before returning

    return (Steinberg::Vst::IEditController*)new sandbox_controller(_proxy_data);
}
