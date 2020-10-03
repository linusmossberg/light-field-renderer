#include "light-field-renderer.hpp"

#include <exception>
#include <iostream>

#include <nanogui/nanogui.h>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../shaders/light-field-renderer.vert"
#include "../shaders/light-field-renderer.frag"
#include "../shaders/data-camera-projections.vert"
#include "../shaders/screen.vert"
#include "../shaders/normalize-aperture-filters.frag"

#include "config.hpp"
#include "camera-array.hpp"
#include "../gl-util/fbo.hpp"

LightFieldRenderer::LightFieldRenderer(Widget* parent, const glm::ivec2 &fixed_size, const std::shared_ptr<Config> &cfg)
    : Canvas(parent, 1, false), draw_shader(screen_vert, normalize_aperture_filters_frag), quad(), cfg(cfg)
{
    set_fixed_size({ fixed_size.x, fixed_size.y });
    fb_size = fixed_size;
    if (draw_border())
    {
        fb_size -= 2;
    }
    fb_size = glm::ivec2(glm::vec2(fb_size) * screen()->pixel_ratio());

    fbo = std::make_unique<FBO>(fb_size);
}

void LightFieldRenderer::draw_contents()
{
    if (!camera_array || !shader) return;

    move();

    fbo->bind();
    quad.bind();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    glDisable(GL_CULL_FACE);

    shader->use();

    auto view = glm::lookAt(eye, eye + forward, up);

    float aspect = fb_size.x / (float)fb_size.y;
    float fov_y = 2.0f * std::atan2f(0.5f * (cfg->sensor_width / aspect), cfg->focal_length);
    auto projection = glm::perspective(fov_y, aspect, 0.001f, 10.f);

    // Flip projection if uv plane is behind camera
    if (eye.z < 0.0) projection = glm::scale(projection, glm::vec3(-1));

    auto VP = projection * view;

    glUniformMatrix4fv(shader->getLocation("VP"), 1, GL_FALSE, &VP[0][0]);
    glUniform3fv(shader->getLocation("eye"), 1, &eye[0]);
    glUniform1f(shader->getLocation("focus_distance"), cfg->focus_distance);
    glUniform1f(shader->getLocation("aperture_diameter"), cfg->focal_length / cfg->f_stop);
    glUniform3fv(shader->getLocation("forward"), 1, &forward[0]);
    glUniform3fv(shader->getLocation("right"), 1, &right[0]);
    glm::vec3 real_up = glm::normalize(glm::cross(-right, forward));
    glUniform3fv(shader->getLocation("up"), 1, &real_up[0]);
    glUniform1f(shader->getLocation("aperture_falloff"), cfg->aperture_falloff);

    for (int i = 0; i < camera_array->cameras.size(); i++)
    {
        if (!camera_array->light_slab)
        {
            camera_array->bind(i, shader->getLocation("data_eye"), shader->getLocation("data_VP"));
        }
        else
        {
            camera_array->bind(i, shader->getLocation("data_eye"));
            glm::vec2 st_size(camera_array->cameras[i].size);
            st_size = (st_size / st_size.x) * (float)cfg->st_width;
            glUniform2fv(shader->getLocation("st_size"), 1, &st_size[0]);
            glUniform1f(shader->getLocation("st_distance"), cfg->st_distance);
        }   
        quad.draw();
    }


    float max_weight_sum = normalize_aperture ? 1.0f : fbo->getMaxAlpha();

    fbo->unBind();
    fbo->bindTexture();
    draw_shader.use();

    glUniform1f(draw_shader.getLocation("max_weight_sum"), max_weight_sum);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    quad.draw();
}

void LightFieldRenderer::move()
{
    eye = glm::vec3(cfg->x, cfg->y, cfg->z);

    if(!target_movement)
    {
        forward = glm::vec3( std::sin(cfg->yaw),
                            -std::sin(cfg->pitch) * std::cos(cfg->yaw),
                            -std::cos(cfg->pitch) * std::cos(cfg->yaw));
    }
    
    right = glm::normalize(glm::cross(forward, up));

    double current_time = glfwGetTime();
    if (current_time > last_time)
    {
        float scale = glm::length(camera_array->uv_size) * (float)(current_time - last_time);

        if (moves[FORWARD]) eye += forward * scale;
        if (moves[BACK])    eye -= forward * scale;
        if (moves[RIGHT])   eye += right * scale;
        if (moves[LEFT])    eye -= right * scale;
        if (moves[UP])      eye += up * scale;
        if (moves[DOWN])    eye -= up * scale;

        cfg->x = eye.x;
        cfg->y = eye.y;
        cfg->z = eye.z;

        eye = glm::vec3(cfg->x, cfg->y, cfg->z);

        if (target_movement)
        {
            forward = glm::normalize(glm::vec3(cfg->target_x - eye.x, cfg->target_y - eye.y, cfg->target_z - eye.z));
            cfg->yaw = std::asin(forward.x);
            cfg->pitch = std::atan2(-forward.y, -forward.z);
        }
    }
    last_time = current_time;
}

void LightFieldRenderer::open()
{
    try
    {
        camera_array = std::make_unique<CameraArray>(cfg->folder);

        if (!camera_array->light_slab)
        {
            shader = std::make_unique<MyShader>(
                (std::string(light_field_renderer_vert) + std::string(perspective_projection)).c_str(),
                light_field_renderer_frag
            );
        }
        else
        {
            shader = std::make_unique<MyShader>(
                (std::string(light_field_renderer_vert) + std::string(light_slab_projection)).c_str(),
                light_field_renderer_frag
            );
        }
    }
    catch (const std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
        camera_array.reset();
        shader.reset();
    }
}

bool LightFieldRenderer::mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers)
{
    if (mouse_navigation)
    {
        if (target_movement)
        {
            float scale = glm::length(camera_array->uv_size);
            cfg->x += scale * rel.x() / (float)fb_size.x;
            cfg->y -= scale * rel.y() / (float)fb_size.y;
            forward = glm::normalize(glm::vec3(cfg->target_x - cfg->x, cfg->target_y -cfg->y, cfg->target_z - cfg->z));
        }
        else
        {
            cfg->pitch += rel.y() / (float)fb_size.y;
            cfg->yaw += rel.x() / (float)fb_size.x;
        }
    }
    return true;
}

bool LightFieldRenderer::scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel)
{
    cfg->focus_distance += rel.y() * (cfg->focus_distance.getRange() / 50.0f);
    return true;
}

bool LightFieldRenderer::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers)
{
    mouse_navigation = down && camera_array;

    if (mouse_navigation)
        glfwSetInputMode(screen()->glfw_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else
        glfwSetInputMode(screen()->glfw_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    return true;
}

void LightFieldRenderer::keyboardEvent(int key, int scancode, int action, int modifiers)
{
    bool pressed;
    switch (action)
    {
    case GLFW_PRESS: pressed = true; break;
    case GLFW_RELEASE: pressed = false; break;
    default: return;
    }

    switch (key)
    {
    case GLFW_KEY_W: moves[FORWARD] = pressed; break;
    case GLFW_KEY_S: moves[BACK] = pressed; break;
    case GLFW_KEY_A: moves[LEFT] = pressed; break;
    case GLFW_KEY_D: moves[RIGHT] = pressed; break;
    case GLFW_KEY_SPACE: moves[UP] = pressed; break;
    case GLFW_KEY_LEFT_CONTROL: moves[DOWN] = pressed; break;
    }
}