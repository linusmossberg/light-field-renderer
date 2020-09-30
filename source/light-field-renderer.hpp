#pragma once

#include <filesystem>

#include <nanogui/canvas.h>

#include <glm/glm.hpp>

#include "shader.hpp"
#include "quad.hpp"

class CameraArray;
class FBO;
class Config;

class LightFieldRenderer : public nanogui::Canvas
{
public:
    LightFieldRenderer(Widget* parent, const glm::ivec2 &fixed_size, const std::shared_ptr<Config> &cfg);

    virtual void draw_contents() override;

    void performMovement();

    void open();

    virtual bool mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers) override;
    virtual bool scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) override;
    virtual bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
    void keyboardEvent(int key, int scancode, int action, int modifiers);

    glm::vec3 forward, right, eye;
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::ivec2 fb_size;

    double last_time = std::numeric_limits<double>::max();

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

    bool navigation = false;

private:
    std::shared_ptr<Config> cfg;
    std::unique_ptr<CameraArray> camera_array;
    std::unique_ptr<MyShader> shader;
    MyShader draw_shader;
    Quad quad;
    std::unique_ptr<FBO> fbo;
};
