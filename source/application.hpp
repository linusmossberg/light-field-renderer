#include <nanogui/nanogui.h>

#include "config.hpp"

class LightFieldRenderer;

class Application : public nanogui::Screen 
{
public:
    Application();

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers);

    virtual void draw(NVGcontext *ctx);

private:
    LightFieldRenderer *light_field_renderer;
    std::shared_ptr<Config> cfg;

    struct PropertySlider
    {
        PropertySlider(nanogui::Slider* slider, Config::Property* prop)
            : slider(slider), prop(prop), last_value(prop->get()) { }

        nanogui::Slider* slider;
        Config::Property* prop;

        float last_value;

        void updateSliderValue()
        {
            if (std::abs(prop->get() - last_value) > 1e-6f)
            {
                float v = prop->getNormalized();
                slider->set_value(v);
                slider->callback()(v);
                last_value = prop->get();
            }
        }
    };

    std::vector<PropertySlider> sliders;
};