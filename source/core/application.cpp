#include "application.hpp"

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "light-field-renderer.hpp"

Application::Application() : 
    Screen(nanogui::Vector2i(1470, 750), "Light Field Renderer", true, false, false, false, false, 3U, 3U), 
    cfg(std::make_shared<Config>())
{
    inc_ref();

    auto theme = new nanogui::Theme(this->nvg_context());
    theme->m_window_fill_focused = nanogui::Color(45, 255);
    theme->m_window_fill_unfocused = nanogui::Color(45, 255);
    theme->m_drop_shadow = nanogui::Color(0, 0);

    nanogui::Window *window;
    nanogui::Button* b;
    nanogui::Widget* panel;
    nanogui::Label* label;

    window = new nanogui::Window(this, "Render");
    window->set_position(nanogui::Vector2i(410, 10));
    window->set_layout(new nanogui::GroupLayout());
    window->set_theme(theme);

    light_field_renderer = new LightFieldRenderer(window, cfg);
    light_field_renderer->set_visible(true);

    b = new nanogui::Button(window->button_panel(), "", FA_CAMERA);
    b->set_tooltip("Save Screenshot");
    b->set_callback([this]
    {
        std::string path = nanogui::file_dialog({{"tga", ""}}, true);
        if (path.empty()) return;
        light_field_renderer->saveNextRender(path);
    });

    window = new nanogui::Window(this, "Menu");
    window->set_position({ 10, 10 });
    window->set_layout(new nanogui::GroupLayout(15, 6, 15, 0));
    window->set_theme(theme);

    b = new nanogui::Button(window->button_panel(), "", FA_QUESTION);
    b->set_font_size(15);
    b->set_tooltip("Controls");

    panel = new nanogui::Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2, nanogui::Alignment::Fill, 0, 5));

    label = new nanogui::Label(panel, "Light Field", "sans-bold");
    label->set_fixed_width(86);

    b = new nanogui::Button(panel, "Open", FA_FOLDER_OPEN);
    b->set_fixed_size({ 270, 20 });
    b->set_callback([this]
    {
        std::string path = nanogui::file_dialog
        (
            { 
                {"cfg", ""}, {"jpeg", ""}, { "jpg","" }, {"png",""}, 
                {"tga",""}, {"bmp",""}, {"psd",""}, {"gif",""}, 
                {"hdr",""}, {"pic",""}, {"pnm", ""} 
            }, false
        );

        if (path.empty()) return;

        try
        {
            cfg->open(path);
            light_field_renderer->open();

            light_field_renderer->resize();
            perform_layout();
        }
        catch (const std::exception &ex)
        {
            std::cout << ex.what() << std::endl;
        }
    });


    panel = new nanogui::Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 4, nanogui::Alignment::Fill));
    label = new nanogui::Label(panel, "Render Size", "sans-bold");
    label->set_fixed_width(86);

    float_box_rows.push_back(PropertyBoxRow(panel, { &cfg->width, &cfg->height }, "", "px", 0, 1.0f, "", 180));

    b = new nanogui::Button(panel, "Set");
    b->set_font_size(16);
    b->set_fixed_size({ 90, 20 });
    b->set_callback([this, window]
        { 
            light_field_renderer->resize();
            perform_layout();
        }
    );

    sliders.emplace_back(window, &cfg->exposure, "Exposure", "EV", 1);

    new nanogui::Label(window, "Optics", "sans-bold", 20);
    sliders.emplace_back(window, &cfg->focal_length, "Focal Length", "mm", 2);
    sliders.emplace_back(window, &cfg->sensor_width, "Sensor Width", "mm", 2);
    sliders.emplace_back(window, &cfg->focus_distance, "Focus Distance", "m", 2);

    sliders.emplace_back(window, &cfg->f_stop, "F-Stop", "", 2);
    sliders.emplace_back(window, &cfg->aperture_falloff, "Filter Falloff", "", 2);

    panel = new Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 3, nanogui::Alignment::Fill, 0, 5));

    label = new nanogui::Label(panel, "Option", "sans-bold");
    label->set_fixed_width(86);

    nanogui::Button* normalize = new nanogui::Button(panel, "Normalize Aperture");
    normalize->set_fixed_size({ 125, 20 });
    normalize->set_font_size(14);
    normalize->set_tooltip("Normalize aperture filter weights per pixel. Disabling this results in vignetting.");
    normalize->set_flags(nanogui::Button::Flags::ToggleButton);
    normalize->set_pushed(light_field_renderer->normalize_aperture);
    normalize->set_change_callback([this](bool state)
    {
        light_field_renderer->normalize_aperture = state;
    });

    nanogui::Button* breathing = new nanogui::Button(panel, "Focus Breathing");
    breathing->set_fixed_size({ 125, 20 });
    breathing->set_font_size(14);
    breathing->set_tooltip("Factor in the focus distance to the image distance according to the thin-lens equation.");
    breathing->set_flags(nanogui::Button::Flags::ToggleButton);
    breathing->set_pushed(light_field_renderer->focus_breathing);
    breathing->set_change_callback([this](bool state)
    {
        light_field_renderer->focus_breathing = state;
    });

    new nanogui::Label(window, "Navigation", "sans-bold", 20);

    panel = new nanogui::Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 4, nanogui::Alignment::Fill, 0, 5));

    label = new nanogui::Label(panel, "Mode", "sans-bold");
    label->set_fixed_width(86);

    nanogui::Button* free = new nanogui::Button(panel, "Free", FA_STREET_VIEW);
    free->set_flags(nanogui::Button::Flags::ToggleButton);
    free->set_pushed(light_field_renderer->navigation == LightFieldRenderer::Navigation::FREE);
    free->set_fixed_size({ 83, 20 });
    free->set_font_size(16);
    free->set_tooltip("The camera is rotated freely with the mouse.");

    nanogui::Button* target = new nanogui::Button(panel, "Target", FA_BULLSEYE);
    target->set_flags(nanogui::Button::Flags::ToggleButton);
    target->set_pushed(light_field_renderer->navigation == LightFieldRenderer::Navigation::TARGET);
    target->set_fixed_size({ 83, 20 });
    target->set_font_size(16);
    target->set_tooltip("The camera looks at the target coordinate and the mouse controls the camera position.");

    nanogui::Button* animate = new nanogui::Button(panel, "Animate", FA_BEZIER_CURVE);
    animate->set_flags(nanogui::Button::Flags::ToggleButton);
    animate->set_pushed(light_field_renderer->navigation == LightFieldRenderer::Navigation::ANIMATE);
    animate->set_fixed_size({ 83, 20 });
    animate->set_font_size(16);

    free->set_change_callback
    (
        [this, free, target, animate](bool state)
        {
            light_field_renderer->navigation = LightFieldRenderer::Navigation::FREE;
            free->set_pushed(true);
            target->set_pushed(false);
            animate->set_pushed(false);
        }
    );
    target->set_change_callback
    (
        [this, free, target, animate](bool state)
        {
            light_field_renderer->navigation = LightFieldRenderer::Navigation::TARGET;
            free->set_pushed(false);
            target->set_pushed(true);
            animate->set_pushed(false);
        }
    );
    animate->set_change_callback
    (
        [this, free, target, animate](bool state)
        {
            light_field_renderer->navigation = LightFieldRenderer::Navigation::ANIMATE;
            free->set_pushed(false);
            target->set_pushed(false);
            animate->set_pushed(true);
        }
    );

    float_box_rows.push_back(PropertyBoxRow(window, { &cfg->x, &cfg->y, &cfg->z }, "Position", "m", 3, 0.1f));
    float_box_rows.push_back(PropertyBoxRow(window, { &cfg->yaw, &cfg->pitch }, "Rotation", "°", 1, 1.0f));
    float_box_rows.push_back(PropertyBoxRow(window, { &cfg->target_x, &cfg->target_y, &cfg->target_z }, "Target", "m", 3, 1.0f));

    sliders.emplace_back(window, &cfg->speed, "Speed", "m/s", 2);

    panel = new nanogui::Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2, nanogui::Alignment::Fill, 0, 5));

    label = new nanogui::Label(panel, " ", "sans-bold");
    label->set_fixed_width(86);

    nanogui::PopupButton *popup_btn = new nanogui::PopupButton(panel, "Animation Settings");
    popup_btn->set_font_size(16);
    popup_btn->set_fixed_size({ 270, 20 });
    nanogui::Popup *popup = popup_btn->popup();
    popup->set_layout(new nanogui::GroupLayout());

    sliders.emplace_back(popup, &cfg->animation_sway, "Sway", "", 2);
    sliders.emplace_back(popup, &cfg->animation_depth, "Depth", "", 2);
    sliders.emplace_back(popup, &cfg->animation_cycles, "Cycles", "", 0);
    sliders.emplace_back(popup, &cfg->animation_scale, "Scale", "", 2);
    sliders.emplace_back(popup, &cfg->animation_duration, "Loop Duration", "s", 2);

    new nanogui::Label(window, "Autofocus", "sans-bold", 20);

    panel = new Widget(window);
    panel->set_layout(new nanogui::GridLayout(nanogui::Orientation::Horizontal, 4, nanogui::Alignment::Fill, 0, 5));

    label = new nanogui::Label(panel, "Option", "sans-bold");
    label->set_fixed_width(86);

    nanogui::Button* focus_af = new nanogui::Button(panel, "Focus");
    focus_af->set_fixed_size({ 83, 20 });
    focus_af->set_font_size(14);
    focus_af->set_tooltip("Can also be initiated by using shift+click in the render view");
    focus_af->set_callback([this]()
    {
        light_field_renderer->autofocus_click = true;
    });

    nanogui::Button* continuous_af = new nanogui::Button(panel, "Continuous");
    continuous_af->set_flags(nanogui::Button::Flags::ToggleButton);
    continuous_af->set_font_size(14);
    continuous_af->set_pushed(light_field_renderer->continuous_autofocus);
    continuous_af->set_fixed_size({ 83, 20 });
    continuous_af->set_change_callback([this](bool state)
    {
        light_field_renderer->continuous_autofocus = state;
    });

    nanogui::Button* visualize_af = new nanogui::Button(panel, "Visualize");
    visualize_af->set_flags(nanogui::Button::Flags::ToggleButton);
    visualize_af->set_pushed(light_field_renderer->visualize_autofocus);
    visualize_af->set_font_size(14);
    visualize_af->set_fixed_size({ 83, 20 });
    visualize_af->set_change_callback([this](bool state)
    {
        light_field_renderer->visualize_autofocus = state;
    });

    float_box_rows.push_back(PropertyBoxRow(
        window, { &cfg->autofocus_x, &cfg->autofocus_y }, "Screen Point", "", 2, 0.01f, 
        "Can also be set using shift+click in the render view")
    );
    float_box_rows.push_back(PropertyBoxRow(
        window, { &cfg->template_size }, "Template Size", "px", 0, 2.0f, 
        "Size of the focus region.")
    );
    float_box_rows.push_back(PropertyBoxRow(
        window, { &cfg->search_scale }, "Search Scale", "", 1, 0.1f, 
        "Scale of search region relative to the focus region")
    );

    label = new nanogui::Label(window, "Light Slab", "sans-bold", 20);
    label->set_tooltip("Scale of rectified light fields. Has no effect on unrectified light fields.");
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
