#include <sandbox.hpp>

struct sandbox_controller : public Steinberg::Vst::EditControllerEx1 {

    // sandbox_controller(original_plugin_data* original_processor)
    sandbox_controller()
    {
    }

    ~sandbox_controller() override
    {
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override
    {

        return EditControllerEx1::initialize(context);
    }

    Steinberg::tresult PLUGIN_API terminate() override
    {
        return EditControllerEx1::terminate();
    }

    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override
    {
        return Steinberg::kResultOk;
    }

    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override
    {
        return nullptr;
    }

    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override
    {
        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override
    {
        return Steinberg::kResultOk;
    }

    // //---Interface--------- PAS CLAIR SI ON A BESOIN
    // DEFINE_INTERFACES
    // // Here you can add more supported VST3 interfaces
    // // DEF_INTERFACE (Vst::IXXX)
    // END_DEFINE_INTERFACES(EditController)
    // DELEGATE_REFCOUNT(EditController)
};


Steinberg::FUnknown* create_sandbox_controller_instance(void* context)
{
    // return (Steinberg::Vst::IEditController*)new sandbox_controller((original_plugin_data*)context);
    return (Steinberg::Vst::IEditController*)new sandbox_controller();
}
