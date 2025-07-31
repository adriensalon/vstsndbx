#include <sandbox.hpp>

Steinberg::Vst::IComponentHandler* _hostHandler = nullptr;

sandbox_controller::sandbox_controller(const sandboxed_proxy_data& proxy_data)
    : _proxy_data(proxy_data)
{
}

sandbox_controller::~sandbox_controller()
{
}

Steinberg::tresult PLUGIN_API sandbox_controller::initialize(Steinberg::FUnknown* context)
{
    // Query the host's IComponentHandler
    // _hostHandler = nullptr;
    // if (context)
    //     context->queryInterface(Steinberg::Vst::IComponentHandler::iid, (void**)&_hostHandler);


    if (EditControllerEx1::initialize(context) != Steinberg::kResultOk) {
        std::cout << "Sandbox error: kResultFalse from base proxy class EditControllerEx1::initialize" << std::endl;
        return Steinberg::kResultFalse;
    }
    _hostHandler = getComponentHandler();

    for (const Steinberg::Vst::UnitInfo& _unit_info : _proxy_data.plugin_data->original_units) {
        if (_unit_info.id == Steinberg::Vst::kRootUnitId && _unit_info.name[0] == 0)
            continue;

        Steinberg::Vst::Unit* _proxy_unit = new Steinberg::Vst::Unit(_unit_info);
        addUnit(_proxy_unit);
    }

    for (const Steinberg::Vst::ParameterInfo& _parameter_info : _proxy_data.plugin_data->original_parameters) {
        parameters.addParameter(_parameter_info);
    }

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API sandbox_controller::terminate()
{
    return EditControllerEx1::terminate();
}

Steinberg::tresult PLUGIN_API sandbox_controller::setComponentState(Steinberg::IBStream* state)
{
    if (_proxy_data.get_sandboxed_controller()->setComponentState(state) != Steinberg::kResultOk) {
        //
    }
    return Steinberg::kResultOk;
}

Steinberg::IPlugView* PLUGIN_API sandbox_controller::createView(Steinberg::FIDString name)
{
    return _proxy_data.get_sandboxed_controller()->createView(name);
}

Steinberg::tresult PLUGIN_API sandbox_controller::setState(Steinberg::IBStream* state)
{
    if (_proxy_data.get_sandboxed_controller()->setState(state) != Steinberg::kResultOk) {
        //
    }
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API sandbox_controller::getState(Steinberg::IBStream* state)
{
    if (_proxy_data.get_sandboxed_controller()->getState(state) != Steinberg::kResultOk) {
        //
    }
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API sandbox_controller::setParamNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value)
{
    Steinberg::FUnknownPtr<Steinberg::Vst::IEditController> _edit_controller(_proxy_data.get_sandboxed_controller());
    if (!_edit_controller) {
        std::cout << "Sandbox error: interface IEditController not implemented by sandboxed class" << std::endl;
        return Steinberg::kResultFalse;
    }
    if (_edit_controller->setParamNormalized(id, value) != Steinberg::kResultOk) {
        std::cout << "Sandbox error: kResultFalse from sandboxed class IEditController::setParamNormalized" << std::endl;
        return Steinberg::kResultFalse;
    }

    return Steinberg::kResultOk;
}

Steinberg::Vst::ParamValue PLUGIN_API sandbox_controller::getParamNormalized(Steinberg::Vst::ParamID id)
{
    Steinberg::FUnknownPtr<Steinberg::Vst::IEditController> _edit_controller(_proxy_data.get_sandboxed_controller());
    if (!_edit_controller) {
        std::cout << "Sandbox error: interface IEditController not implemented by sandboxed class" << std::endl;
        return Steinberg::kResultFalse;
    }

    return _edit_controller->getParamNormalized(id);
}

Steinberg::tresult sandbox_controller::beginEdit(Steinberg::Vst::ParamID tag)
{
    if (_hostHandler)
        return _hostHandler->beginEdit(tag);
    return Steinberg::kResultOk;
}

Steinberg::tresult sandbox_controller::performEdit(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized)
{
    if (_hostHandler)
        return _hostHandler->performEdit(tag, valueNormalized);
    return Steinberg::kResultOk;
}

Steinberg::tresult sandbox_controller::endEdit(Steinberg::Vst::ParamID tag)
{
    if (_hostHandler)
        return _hostHandler->endEdit(tag);
    return Steinberg::kResultOk;
}

Steinberg::tresult sandbox_controller::startGroupEdit()
{
    Steinberg::Vst::IComponentHandler2* handler2 = nullptr;
    if (_hostHandler && _hostHandler->queryInterface(Steinberg::Vst::IComponentHandler2::iid, (void**)&handler2) == Steinberg::kResultOk && handler2)
    {
        Steinberg::tresult result = handler2->startGroupEdit();
        handler2->release();
        return result;
    }
    return Steinberg::kResultOk;
}

Steinberg::tresult sandbox_controller::finishGroupEdit()
{
    Steinberg::Vst::IComponentHandler2* handler2 = nullptr;
    if (_hostHandler && _hostHandler->queryInterface(Steinberg::Vst::IComponentHandler2::iid, (void**)&handler2) == Steinberg::kResultOk && handler2)
    {
        Steinberg::tresult result = handler2->finishGroupEdit();
        handler2->release();
        return result;
    }
    return Steinberg::kResultOk;
}
