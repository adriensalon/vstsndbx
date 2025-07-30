#include <sandbox.hpp>

std::shared_ptr<sandboxed_plugin_instance> get_or_create_pending_instance(sandboxed_plugin_data* data, std::size_t& id)
{
    std::lock_guard _lock(data->pending_mutex);

    if (data->should_increment.exchange(false)) {
        id = data->next_instance_id.fetch_add(1);
    } else {
        id = data->next_instance_id.load();
    }

    std::shared_ptr<sandboxed_plugin_instance>& _instance_slot = data->sandboxed_instances[id];
    if (!_instance_slot) {
        _instance_slot = std::make_shared<sandboxed_plugin_instance>();
    }

    return _instance_slot;
}

Steinberg::Vst::IComponent* sandboxed_proxy_data::get_sandboxed_processor()
{
    return plugin_data->sandboxed_instances[instance_id]->sandboxed_processor;
}

Steinberg::Vst::IEditController* sandboxed_proxy_data::get_sandboxed_controller()
{
    return plugin_data->sandboxed_instances[instance_id]->sandboxed_controller;
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
    _plugin_instance->host_module = VST3::Hosting::Module::create(_proxy_data.plugin_data->plugin_path.string(), _module_error);
    if (!_plugin_instance->host_module) {
        std::string _error_text = "Sandbox error: could not create Module with error " + _module_error;
        std::cerr << _error_text << std::endl;
        throw std::runtime_error(_error_text);
    }
    _plugin_instance->plugin_provider = std::make_shared<Steinberg::Vst::PlugProvider>(_plugin_instance->host_module->getFactory(), _proxy_data.plugin_data->class_info, true);
    _plugin_instance->sandboxed_processor = _plugin_instance->plugin_provider->getComponent();
    _plugin_instance->sandboxed_controller = _plugin_instance->plugin_provider->getController();
    if (!_plugin_instance->sandboxed_processor || !_plugin_instance->sandboxed_controller) {
        std::string _error_text = "Sandbox error: could not create sandboxed processor or controller";
        std::cerr << _error_text << std::endl;
        throw std::runtime_error(_error_text);
    }
    _plugin_instance->is_proxy_processor_created = true;
    _plugin_instance->sandboxed_processor->addRef(); // Must addRef before returning
    _plugin_instance->sandboxed_controller->addRef(); // Must addRef before returning

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
    _plugin_instance->is_proxy_controller_created = true;

    return (Steinberg::Vst::IEditController*)new sandbox_controller(_proxy_data);
}
