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
        PropertySlider(nanogui::Widget* window, Config::Property* p, const std::string &name, 
                       const std::string &unit, size_t precision);

        nanogui::Slider* slider;
        Config::Property* const prop;

        float last_value;

        void updateValue();
    };

    struct PropertyBoxRow
    {
        PropertyBoxRow(nanogui::Widget* window, const std::vector<Config::Property*> &properties, 
                       const std::string &name, const std::string &unit, size_t precision, float step);

        std::vector<Config::Property*> properties;
        std::vector<nanogui::FloatBox<float>*> float_boxes;
        std::vector<float> last_values;

        void updateValues();
    };

    std::vector<PropertySlider> sliders;
    std::vector<PropertyBoxRow> float_box_rows;
};