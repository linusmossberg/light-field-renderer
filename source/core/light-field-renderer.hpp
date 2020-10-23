#pragma once

#include <filesystem>

#include <nanogui/canvas.h>

#include <glm/glm.hpp>

#include "../gl-util/shader.hpp"
#include "../gl-util/quad.hpp"
#include "../gl-util/n-sided-polygon.hpp"

class CameraArray;
class FBO;
class Config;

class LightFieldRenderer : public nanogui::Canvas
{
public:
    LightFieldRenderer(Widget* parent, const std::shared_ptr<Config> &cfg);

    virtual void draw_contents() override;

    void move();
    void open();
    void resize();
    void saveNextRender(const std::string& filename);

    virtual bool mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers) override;
    virtual bool scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel) override;
    virtual bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
    void keyboardEvent(int key, int scancode, int action, int modifiers);

    glm::vec3 forward, right, up, eye;
    glm::mat4 VP;
    const glm::vec3 Y_AXIS = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::ivec2 fb_size;
    float image_distance;

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

    enum Navigation
    {
        FREE,
        TARGET,
        ANIMATE
    };

    Navigation navigation = Navigation::FREE;

    bool mouse_active = false;
    bool normalize_aperture = true;
    bool continuous_autofocus = false;
    bool autofocus_click = false;
    bool focus_breathing = false;

    bool visualize_autofocus = false;

    // Used to prevent large relative movement the first click
    bool click = false;

private:
    // The following functions are implemented in phase-detect-autofocus.cpp
    void phaseDetectAutofocus();
    glm::vec3 pixelDirection(const glm::vec2 &px);
    glm::vec3 pixelToFocalPlane(const glm::vec2 &px);
    glm::vec2 pixelToCameraPlane(const glm::vec2 &px);
    std::vector<float> sqdiff_data;

    std::shared_ptr<Config> cfg;
    std::unique_ptr<CameraArray> camera_array;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Shader> disparity_shader;
    Shader draw_shader;
    Shader visualize_autofocus_shader;
    Shader template_match_shader;
    Quad quad;
    NSidedPolygon aperture;
    std::unique_ptr<FBO> fbo0;
    std::unique_ptr<FBO> fbo1;

    void saveRender();
    bool save_next = false;
    std::string savename = "";

    void animate();
};
