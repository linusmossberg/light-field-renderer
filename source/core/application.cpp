#include "application.hpp"

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "light-field-renderer.hpp"

using namespace nanogui;

Application::Application() : 
    Screen(Vector2i(1280, 720), "Light Field Renderer", true, false, false, false, false, 3U, 3U), 
    cfg(std::make_shared<Config>())
{
    inc_ref();

    auto theme = new nanogui::Theme(this->nvg_context());
    theme->m_window_fill_focused = Color(45, 255);
    theme->m_window_fill_unfocused = Color(45, 255);
    theme->m_drop_shadow = Color(0, 0);

    Window *window;
    Button* b;

    window = new Window(this, "Render");
    window->set_position(Vector2i(600, 50));
    window->set_layout(new GroupLayout());
    window->set_theme(theme);

    light_field_renderer = new LightFieldRenderer(window, glm::ivec2(512), cfg);
    light_field_renderer->set_visible(true);

    window = new Window(this, "Menu");
    window->set_position(Vector2i(50, 50));
    window->set_layout(new GroupLayout(15, 6, 15, 0));
    window->set_theme(theme);

    b = new Button(window->button_panel(), "", FA_QUESTION);
    b->set_font_size(15);
    b->set_tooltip("Controls");

    new Label(window, "Add Light Field", "sans-bold", 20);

    b = new Button(window, "Open", FA_FOLDER_OPEN);
    b->set_callback([this]
    {
        std::string path = file_dialog({ {"cfg", "Configuration File"}, {"jpg", "Configuration File"} }, false);
        if (path.empty()) return;
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

    new Label(window, "Optics", "sans-bold", 20);
    sliders.emplace_back(window, &cfg->focal_length, "Focal Length", "mm", 1);
    sliders.emplace_back(window, &cfg->sensor_width, "Sensor Width", "mm", 1);
    sliders.emplace_back(window, &cfg->focus_distance, "Focus Distance", "m", 1);

    new Label(window, "Aperture", "sans-bold", 20);
    sliders.emplace_back(window, &cfg->f_stop, "F-Stop", "", 1);
    sliders.emplace_back(window, &cfg->aperture_falloff, "Filter Falloff", "", 2);

    Widget* panel = new Widget(window);
    panel->set_layout(new GridLayout(Orientation::Horizontal, 2, Alignment::Fill));
    panel->set_fixed_width(200);

    Label* label = new Label(panel, "Normalize Aperture Filter", "sans-bold");
    label->set_fixed_width(150);

    CheckBox *cb = new CheckBox(panel, "",
        [this](bool state) { light_field_renderer->normalize_aperture = state; }
    );
    cb->set_checked(light_field_renderer->normalize_aperture);
    cb->set_fixed_width(50);

    new Label(window, "Navigation", "sans-bold", 20);

    panel = new Widget(window);
    panel->set_layout(new GridLayout(Orientation::Horizontal, 3, Alignment::Fill, 0, 5));

    label = new Label(panel, "Mode", "sans-bold");
    label->set_fixed_width(86);

    Button* free = new Button(panel, "Free", FA_STREET_VIEW);
    free->set_flags(Button::Flags::ToggleButton);
    free->set_pushed(!light_field_renderer->target_movement);
    free->set_fixed_size({ 125, 20 });
    free->set_tooltip("The camera is rotated freely with the mouse.");

    Button* target = new Button(panel, "Target", FA_BULLSEYE);
    target->set_flags(Button::Flags::ToggleButton);
    target->set_pushed(light_field_renderer->target_movement);
    target->set_fixed_size({ 125, 20 });
    target->set_tooltip("The camera looks at the center of the scene and the mouse controls the camera position.");

    free->set_callback([this, target]
    {
        light_field_renderer->target_movement = false;
        target->set_pushed(false);
    });
    target->set_callback([this, free]()
    {
        light_field_renderer->target_movement = true;
        free->set_pushed(false);
    });

    float_box_rows.push_back(PropertyBoxRow(window, { &cfg->x, &cfg->y, &cfg->z }, "Position", "m", 3, 0.1f));
    float_box_rows.push_back(PropertyBoxRow(window, { &cfg->yaw, &cfg->pitch }, "Rotation", "°", 1, 1.0f));
    float_box_rows.push_back(PropertyBoxRow(window, { &cfg->target_x, &cfg->target_y, &cfg->target_z }, "Target", "m", 3, 1.0f));

    new Label(window, "Autofocus", "sans-bold", 20);

        panel = new Widget(window);
    panel->set_layout(new GridLayout(Orientation::Horizontal, 4, Alignment::Fill, 0, 5));

    label = new Label(panel, "Option", "sans-bold");
    label->set_fixed_width(86);

    Button* focus_af = new Button(panel, "Focus");
    focus_af->set_fixed_size({ 83, 20 });
    focus_af->set_font_size(14);
    focus_af->set_callback([this]()
    {
        light_field_renderer->autofocus_click = true;
    });

    Button* continuous_af = new Button(panel, "Continuous");
    continuous_af->set_flags(Button::Flags::ToggleButton);
    continuous_af->set_font_size(14);
    continuous_af->set_pushed(light_field_renderer->continuous_autofocus);
    continuous_af->set_fixed_size({ 83, 20 });
    continuous_af->set_change_callback([this](bool state)
    {
        light_field_renderer->continuous_autofocus = state;
    });

    Button* visualize_af = new Button(panel, "Visualize");
    visualize_af->set_flags(Button::Flags::ToggleButton);
    visualize_af->set_pushed(light_field_renderer->visualize_autofocus);
    visualize_af->set_font_size(14);
    visualize_af->set_fixed_size({ 83, 20 });
    visualize_af->set_change_callback([this](bool state)
    {
        light_field_renderer->visualize_autofocus = state;
    });

    float_box_rows.push_back(PropertyBoxRow(
        window, { &cfg->autofocus_x, &cfg->autofocus_y }, "Screen Point", "", 2, 0.01f, 
        "Can be set using shift+click in the render view")
    );
    float_box_rows.push_back(PropertyBoxRow(
        window, { &cfg->template_size }, "Template Size", "px", 0, 2.0f)
    );
    float_box_rows.push_back(PropertyBoxRow(
        window, { &cfg->search_scale }, "Search Scale", "", 1, 0.1f)
    );

    new Label(window, "Light Slab", "sans-bold", 20);
    sliders.emplace_back(window, &cfg->st_width, "ST Width", "m", 1);
    sliders.emplace_back(window, &cfg->st_distance, "ST Distance", "m", 1);

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
        s.updateValue();
    }

    for (auto &t : float_box_rows)
    {
        t.updateValues();
    }

    Screen::draw(ctx);
}