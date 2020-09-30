#include "application.hpp"

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "light-field-renderer.hpp"
#include "constants.hpp"

using namespace nanogui;

Application::Application() : Screen(Vector2i(1280, 720), "Light Field", true, false, false, false, false, 3U, 3U)
{
    inc_ref();

    Window *window;
    Button* b;

    window = new Window(this, "Light Field Render");
    window->set_position(Vector2i(15, 15));
    window->set_layout(new GroupLayout());

    cfg = std::make_shared<Config>();

    light_field_renderer = new LightFieldRenderer(window, glm::ivec2(512), cfg);
    light_field_renderer->set_visible(true);

    window = new Window(this, "Menu");
    window->set_position(Vector2i(710, 15));
    window->set_layout(new GroupLayout(15, 6, 15, 0));

    new Label(window, "Add Light Field", "sans-bold", 20);

    b = new Button(window, "Open", FA_FOLDER_OPEN);
    b->set_callback([this]
    {
        std::string path = file_dialog({ {"cfg", "Configuration File"}, {"jpg", "Configuration File"} }, false);
        try
        {
            cfg->open(path);
            light_field_renderer->open();
        }
        catch (const std::exception &ex)
        {
            std::cout << ex.what() << std::endl;
        }
    });

    auto addSlider = [&](const std::string& name, const std::string& unit, Config::Property& prop, size_t precision)
    {
        Widget* panel = new Widget(window);
        panel->set_layout(new GridLayout(Orientation::Horizontal, 3, Alignment::Middle));

        Label* label = new Label(panel, name, "sans-bold");
        label->set_fixed_width(86);

        sliders.push_back(PropertySlider{ new Slider(panel), &prop });
        auto &slider = sliders.back().slider;

        slider->set_value(prop.getNormalized());
        slider->set_fixed_width(100);

        TextBox* text_box = new TextBox(panel);
        text_box->set_fixed_size(Vector2i(70, 20));
        text_box->set_font_size(14);
        text_box->set_alignment(TextBox::Alignment::Right);
        text_box->set_units(unit);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << prop.getDisplay();
        text_box->set_value(ss.str());
            
        slider->set_callback([text_box, &prop, precision](float value)
            {
                prop.setNormalized(value);

                std::stringstream ss;
                ss << std::fixed << std::setprecision(precision) << prop.getDisplay();
                text_box->set_value(ss.str());
            }
        );
    };

    new Label(window, "Camera Settings", "sans-bold", 20);
    addSlider("Focal Length", "mm", cfg->focal_length, 1);
    addSlider("Sensor Width", "mm", cfg->sensor_width, 1);
    addSlider("F-Stop", "", cfg->f_stop, 1);
    addSlider("Focus Distance", "m", cfg->focus_distance, 1);

    new Label(window, "Camera Transform", "sans-bold", 20);
    addSlider("X", "m", cfg->x, 2);
    addSlider("Y", "m", cfg->y, 2);
    addSlider("Z", "m", cfg->z, 1);

    addSlider("Yaw", "°", cfg->yaw, 1);
    addSlider("Pitch", "°", cfg->pitch, 1);

    auto ls_label = new Label(window, "Light Slab Settings", "sans-bold", 20);
    addSlider("st Width", "m", cfg->st_width, 1);
    addSlider("st Distance", "m", cfg->st_distance, 1);

    perform_layout();
}

bool Application::keyboard_event(int key, int scancode, int action, int modifiers) 
{
    if (Screen::keyboard_event(key, scancode, action, modifiers))
    {
        return true;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) 
    {
        set_visible(false);
        return true;
    }
    light_field_renderer->keyboardEvent(key, scancode, action, modifiers);
    return false;
}

void Application::draw(NVGcontext *ctx)
{
    for (auto &s : sliders)
    {
        s.updateSliderValue();
    }

    Screen::draw(ctx);
}