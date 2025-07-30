#include <sandbox.hpp>

sandbox_controller::sandbox_controller(const sandboxed_proxy_data& proxy_data)
    : _proxy_data(proxy_data)
{
}

sandbox_controller::~sandbox_controller()
{
}

Steinberg::tresult PLUGIN_API sandbox_controller::initialize(Steinberg::FUnknown* context)
{

    auto k = EditControllerEx1::initialize(context);

    std::vector<Steinberg::Vst::ParameterInfo>& _params = _proxy_data.plugin_data->original_parameters;
    for (const auto& info : _params) {
        // parameters.addParameter(info);
    }

    // forward parameters
    return k;
}

Steinberg::tresult PLUGIN_API sandbox_controller::terminate()
{
    return EditControllerEx1::terminate();
}

Steinberg::tresult PLUGIN_API sandbox_controller::setComponentState(Steinberg::IBStream* state)
{
    return Steinberg::kResultOk;
}

Steinberg::IPlugView* PLUGIN_API sandbox_controller::createView(Steinberg::FIDString name)
{
    return nullptr;
}

Steinberg::tresult PLUGIN_API sandbox_controller::setState(Steinberg::IBStream* state)
{
    _proxy_data.plugin_data->sandboxed_instances[_proxy_data.instance_id]->sandboxed_controller->setState(state);
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API sandbox_controller::getState(Steinberg::IBStream* state)
{
    _proxy_data.plugin_data->sandboxed_instances[_proxy_data.instance_id]->sandboxed_controller->setState(state);
    return Steinberg::kResultOk;
}