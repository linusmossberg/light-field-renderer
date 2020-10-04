#pragma once

#include <filesystem>

#include <nanogui/canvas.h>

#include <glm/glm.hpp>

#include "../gl-util/shader.hpp"
#include "../gl-util/quad.hpp"

class CameraArray;
class FBO;
class Config;

class LightFieldRenderer : public nanogui::Canvas
{
public:
    LightFieldRenderer(Widget* parent, const glm::ivec2 &fixed_size, const std::shared_ptr<Config> &cfg);

    virtual void draw_contents() override;

    void move();

    void open();

    void phaseDetectAutoFocus();

    virtual bool mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers) override;
    virtual bool scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) override;
    virtual bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
    void keyboardEvent(int key, int scancode, int action, int modifiers);

    glm::vec3 forward, right, eye;
    const glm::vec3 Y_AXIS = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::ivec2 fb_size;

    double last_time = std::numeric_limits<double>::max();
    float last_contrast = 0.0f;
    bool last_focus_direction = 0;

    enum Move
    {
        FORWARD,
        BACK,
        LEFT,
        RIGHT,
        DOWN,
        UP,
        NUM
    };

    std::vector<bool> moves = std::vector<bool>(Move::NUM, false);

    bool target_movement = false;
    bool mouse_navigation = false;
    bool normalize_aperture = true;

private:
    std::shared_ptr<Config> cfg;
    std::unique_ptr<CameraArray> camera_array;
    std::unique_ptr<MyShader> shader;
    std::unique_ptr<MyShader> af_shader;
    MyShader draw_shader;
    MyShader template_match_shader;
    MyShader debug_shader;
    Quad quad;
    std::unique_ptr<FBO> fbo;
    std::unique_ptr<FBO> af_fbo; // smaller FBO used for autofocus
};
