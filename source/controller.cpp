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
    Steinberg::tresult _retval = EditControllerEx1::initialize(context);

    for (const Steinberg::Vst::UnitInfo& _unit_info : _proxy_data.plugin_data->original_units) {
        if (_unit_info.id == Steinberg::Vst::kRootUnitId && _unit_info.name[0] == 0)
            continue;
        
        Steinberg::Vst::Unit* _proxy_unit = new Steinberg::Vst::Unit(_unit_info);
        addUnit(_proxy_unit);
    }

    for (const Steinberg::Vst::ParameterInfo& _parameter_info : _proxy_data.plugin_data->original_parameters) {
        parameters.addParameter(_parameter_info);
    }

    return _retval;
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