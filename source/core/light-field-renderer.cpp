#include "light-field-renderer.hpp"

#include <exception>
#include <iostream>
#include <fstream>

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

#include "../shaders/autofocus/disparity.vert"
#include "../shaders/autofocus/disparity.frag"
#include "../shaders/autofocus/template-match.frag"
#include "../shaders/autofocus/visualize-autofocus.frag"

#include "config.hpp"
#include "camera-array.hpp"
#include "../gl-util/fbo.hpp"
#include "util.hpp"

LightFieldRenderer::LightFieldRenderer(Widget* parent, const std::shared_ptr<Config> &cfg) : 
    Canvas(parent, 1, false), quad(), cfg(cfg), aperture(32),
    draw_shader(screen_vert, normalize_aperture_filters_frag),
    visualize_autofocus_shader(screen_vert, visualize_autofocus_frag),
    template_match_shader(screen_vert, template_match_frag)
{
    resize();
}

void LightFieldRenderer::draw_contents()
{
    if (!camera_array || !shader) return;

    move();

    if (continuous_autofocus || autofocus_click || visualize_autofocus)
    {
        phaseDetectionAutofocus();
        autofocus_click = false;
    }

    if (visualize_autofocus)
    {
        fbo1->bindTexture();
        visualize_autofocus_shader.use();

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        quad.draw();

        if (save_next) saveRender();

        return;
    }

    fbo0->bind();
    aperture.bind();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    shader->use();

    glUniformMatrix4fv(shader->getLocation("VP"), 1, GL_FALSE, &VP[0][0]);
    glUniform3fv(shader->getLocation("eye"), 1, &eye[0]);
    glUniform1f(shader->getLocation("focus_distance"), cfg->focus_distance);
    glUniform1f(shader->getLocation("aperture_diameter"), cfg->focal_length / cfg->f_stop);
    glUniform3fv(shader->getLocation("forward"), 1, &forward[0]);
    glUniform3fv(shader->getLocation("right"), 1, &right[0]);
    glUniform3fv(shader->getLocation("up"), 1, &up[0]);
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
        aperture.draw();
    }

    float max_weight_sum = normalize_aperture ? 0.0f : fbo0->getMaxAlpha();

    fbo0->unBind();
    fbo0->bindTexture();
    draw_shader.use();

    quad.bind();

    glUniform1f(draw_shader.getLocation("max_weight_sum"), max_weight_sum);
    glUniform1f(draw_shader.getLocation("exposure"), std::pow(2, cfg->exposure));

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    quad.draw();

    if (save_next) saveRender();
}

void LightFieldRenderer::move()
{
    if (navigation == Navigation::ANIMATE)
    {
        animate();
    }

    if(navigation == Navigation::FREE)
    {
        forward = glm::vec3( std::sin(cfg->yaw),
                            -std::sin(cfg->pitch) * std::cos(cfg->yaw),
                            -std::cos(cfg->pitch) * std::cos(cfg->yaw));
    }

    double current_time = glfwGetTime();
    if (current_time > last_time && navigation != Navigation::ANIMATE)
    {
        float dx = cfg->speed * (float)(current_time - last_time);

        eye = glm::vec3(cfg->x, cfg->y, cfg->z);
        right = glm::normalize(glm::cross(forward, Y_AXIS));

        if (moves[FORWARD]) eye += forward * dx;
        if (moves[BACK])    eye -= forward * dx;
        if (moves[RIGHT])   eye += right * dx;
        if (moves[LEFT])    eye -= right * dx;
        if (moves[UP])      eye += Y_AXIS * dx;
        if (moves[DOWN])    eye -= Y_AXIS * dx;

        cfg->x = eye.x;
        cfg->y = eye.y;
        cfg->z = eye.z;
    }
    last_time = current_time;

    eye = glm::vec3(cfg->x, cfg->y, cfg->z);

    if (navigation != Navigation::FREE)
    {
        forward = glm::normalize(glm::vec3(cfg->target_x - eye.x, cfg->target_y - eye.y, cfg->target_z - eye.z));
        cfg->yaw = std::asin(forward.x);
        cfg->pitch = std::atan2(-forward.y, -forward.z);
    }

    auto view = glm::lookAt(eye, eye + forward, Y_AXIS);
    up = glm::vec3(view[0][1], view[1][1], view[2][1]);
    right = glm::vec3(view[0][0], view[1][0], view[2][0]);

    image_distance = focus_breathing ? imageDistance(cfg->focal_length, cfg->focus_distance) : cfg->focal_length;
    auto projection = perspectiveProjection(image_distance, cfg->sensor_width, fb_size);

    VP = projection * view;
}

void LightFieldRenderer::open()
{
    try
    {
        camera_array.reset();
        camera_array = std::make_unique<CameraArray>(cfg->folder);

        if (!camera_array->light_slab)
        {
            shader = std::make_unique<Shader>(
                (std::string(light_field_renderer_vert) + std::string(perspective_projection)).c_str(),
                light_field_renderer_frag
            );
            disparity_shader = std::make_unique<Shader>(
                (std::string(disparity_vert) + std::string(perspective_projection)).c_str(),
                disparity_frag
            );
        }
        else
        {
            shader = std::make_unique<Shader>(
                (std::string(light_field_renderer_vert) + std::string(light_slab_projection)).c_str(),
                light_field_renderer_frag
            );
            disparity_shader = std::make_unique<Shader>(
                (std::string(disparity_vert) + std::string(light_slab_projection)).c_str(),
                disparity_frag
            );
        }
    }
    catch (const std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
        camera_array.reset();
        shader.reset();
        disparity_shader.reset();
    }
}

bool LightFieldRenderer::mouse_drag_event(const nanogui::Vector2i& p, const nanogui::Vector2i& rel, int button, int modifiers)
{
    if (mouse_active && !click)
    {
        if (navigation == Navigation::TARGET)
        {
            cfg->x += 0.5f * cfg->speed * rel.x() / (float)fb_size.x;
            cfg->y -= 0.5f * cfg->speed * rel.y() / (float)fb_size.x;
            forward = glm::normalize(glm::vec3(cfg->target_x - cfg->x, cfg->target_y -cfg->y, cfg->target_z - cfg->z));
        }
        else if(navigation == Navigation::FREE)
        {
            cfg->pitch += rel.y() / (float)fb_size.x;
            cfg->yaw += rel.x() / (float)fb_size.x;
        }
    }
    click = false;
    return true;
}

bool LightFieldRenderer::scroll_event(const nanogui::Vector2i& p, const nanogui::Vector2f& rel)
{
    cfg->focus_distance += rel.y() * (cfg->focus_distance.getRange() / 50.0f);
    return true;
}

bool LightFieldRenderer::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers)
{
    mouse_active = down && camera_array;

    if (mouse_active)
        glfwSetInputMode(screen()->glfw_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else
        glfwSetInputMode(screen()->glfw_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    click = mouse_active;

    if (modifiers == GLFW_MOD_SHIFT && click)
    {
        auto px = p - position();
        px.y() = size().y() - px.y();
        cfg->autofocus_x = px.x() / (float)size().x();
        cfg->autofocus_y = px.y() / (float)size().y();
        autofocus_click = true;
    }

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

void LightFieldRenderer::resize()
{
    fb_size = { cfg->width, cfg->height };
    set_fixed_size({ fb_size.x, fb_size.y });

    if (draw_border())
    {
        fb_size -= 2;
    }
    fb_size = glm::ivec2(glm::vec2(fb_size) * screen()->pixel_ratio());

    fbo0 = std::make_unique<FBO>(fb_size);
    fbo1 = std::make_unique<FBO>(fb_size);
}

void LightFieldRenderer::saveNextRender(const std::string &filename)
{
    savename = std::filesystem::path(filename).replace_extension(".tga").string();
    save_next = true;
}

void LightFieldRenderer::saveRender()
{
    if (!save_next || savename.empty()) return;

    struct HeaderTGA
    {
        HeaderTGA(uint16_t width, uint16_t height)
            : width(width), height(height) {}

    private:
        uint8_t begin[12] = { 0, 0, 2 };
        uint16_t width;
        uint16_t height;
        uint8_t end[2] = { 24, 32 };
    };

    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    // GL_RGBA is required for some reason
    std::vector<glm::u8vec4> data(vp[2] * vp[3]);
    glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGBA, GL_UNSIGNED_BYTE, data.data());

    HeaderTGA header(vp[2], vp[3]);

    std::vector<glm::u8vec3> rearranged(vp[2] * vp[3]);
    for (int y = 0; y < vp[3]; y++)
    {
        for (int x = 0; x < vp[2]; x++)
        {
            const auto &p = data[(vp[3] - 1 - y) * vp[2] + x];
            rearranged[y * vp[2] + x] = { p.b, p.g, p.r };
        }
    }

    std::ofstream image_file(savename, std::ios::binary);
    image_file.write(reinterpret_cast<char*>(&header), sizeof(header));
    image_file.write(reinterpret_cast<char*>(rearranged.data()), rearranged.size() * 3);
    image_file.close();

    save_next = false;
    savename = "";
}

void LightFieldRenderer::animate()
{
    //static int current_frame = 0;
    //savename = std::string("C:\\Users\\Me\\Documents\\TNM089\\light-field-renderer\\test\\") + std::to_string(current_frame) + ".tga";
    //save_next = true;
    //constexpr float num_frames = 30*4.0f;
    //float f = current_frame / num_frames;
    //current_frame = (current_frame + 1) % (int)num_frames;

    float f = (float)glfwGetTime() / cfg->animation_duration;

    float theta = glm::radians(f * 360.0f);
    float phi = glm::radians(f * std::round(cfg->animation_cycles) * 360.0f);
    glm::vec3 r = glm::vec3(camera_array->xy_size, 0.0f) / 2.0f;
    r.z = std::max(r.x, r.y) * cfg->animation_depth;

    r *= (float)cfg->animation_scale;

    cfg->x = r.x * std::cos(phi) * std::sin(theta);
    cfg->y = r.y * std::sin(phi) * std::sin(theta);

    cfg->z = r.z * std::cos(theta);

    cfg->target_x = glm::mix((1.0f - cfg->animation_sway) * cfg->x, (float)cfg->x, (1.0f - (1.0f + std::cos(theta * 2.0f)) / 2.0f));
    cfg->target_y = glm::mix((1.0f - cfg->animation_sway) * cfg->y, (float)cfg->y, (1.0f - (1.0f + std::cos(theta * 2.0f)) / 2.0f));

    float lim = 0.001f;
    if (cfg->z > 0.0f)
    {
        if (cfg->z < lim) cfg->z = lim;
    }
    else
    {
        if (cfg->z > -lim) cfg->z = -lim;
    }

    cfg->target_z = cfg->z - (r.z / (float)cfg->animation_scale) * 4;
}