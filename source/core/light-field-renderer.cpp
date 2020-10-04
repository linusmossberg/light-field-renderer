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
#include "../shaders/autofocus.vert"
#include "../shaders/autofocus.frag"
#include "../shaders/template-match.frag"
#include "../shaders/debug.frag"

#include "config.hpp"
#include "camera-array.hpp"
#include "../gl-util/fbo.hpp"
#include "util.hpp"

LightFieldRenderer::LightFieldRenderer(Widget* parent, const glm::ivec2 &fixed_size, const std::shared_ptr<Config> &cfg) : 
    Canvas(parent, 1, false), 
    draw_shader(screen_vert, normalize_aperture_filters_frag), 
    quad(), cfg(cfg), 
    template_match_shader(screen_vert, template_match_frag),
    debug_shader(screen_vert, debug_frag)
{
    set_fixed_size({ fixed_size.x, fixed_size.y });
    fb_size = fixed_size;
    if (draw_border())
    {
        fb_size -= 2;
    }
    fb_size = glm::ivec2(glm::vec2(fb_size) * screen()->pixel_ratio());

    fbo = std::make_unique<FBO>(fb_size);
    af_fbo = std::make_unique<FBO>(fb_size);
}

void LightFieldRenderer::draw_contents()
{
    if (!camera_array || !shader) return;

    move();

    phaseDetectAutoFocus();

    fbo->bind();
    quad.bind();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    glDisable(GL_CULL_FACE);

    shader->use();

    auto view = glm::lookAt(eye, eye + forward, Y_AXIS);
    auto projection = perspectiveProjection(cfg->focal_length, cfg->sensor_width, fb_size);

    // Flip projection if uv plane is behind camera
    if (eye.z < 0.0) projection = glm::scale(projection, glm::vec3(-1));

    auto VP = projection * view;

    glm::vec3 up(view[0][1], view[1][1], view[2][1]);

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
        quad.draw();
    }

    float max_weight_sum = normalize_aperture ? 1.0f : fbo->getMaxAlpha();

    fbo->unBind();
    fbo->bindTexture();
    //af_fbo->bindTexture();
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
    
    right = glm::normalize(glm::cross(forward, Y_AXIS));

    double current_time = glfwGetTime();
    if (current_time > last_time)
    {
        float scale = glm::length(camera_array->uv_size) * (float)(current_time - last_time);

        if (moves[FORWARD]) eye += forward * scale;
        if (moves[BACK])    eye -= forward * scale;
        if (moves[RIGHT])   eye += right * scale;
        if (moves[LEFT])    eye -= right * scale;
        if (moves[UP])      eye += Y_AXIS * scale;
        if (moves[DOWN])    eye -= Y_AXIS * scale;

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
            af_shader = std::make_unique<MyShader>(
                (std::string(autofocus_vert) + std::string(perspective_projection)).c_str(),
                autofocus_frag
            );
        }
        else
        {
            shader = std::make_unique<MyShader>(
                (std::string(light_field_renderer_vert) + std::string(light_slab_projection)).c_str(),
                light_field_renderer_frag
            );
            af_shader = std::make_unique<MyShader>(
                (std::string(autofocus_vert) + std::string(light_slab_projection)).c_str(),
                autofocus_frag
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

void LightFieldRenderer::phaseDetectAutoFocus()
{
    glm::ivec2 cameras(100, 200);
    const glm::ivec2 af_size(64);
    const glm::ivec2 search_size(128);
    const glm::ivec2 af_pos(af_fbo->size / 2);

    glm::ivec2 template_min(af_pos - af_size / 2);
    glm::ivec2 template_max(af_pos + af_size / 2);
    glm::ivec2 search_min(af_pos - search_size / 2);
    glm::ivec2 search_max(af_pos + search_size / 2);

    auto view = glm::lookAt(eye, eye + forward, Y_AXIS);
    auto projection = perspectiveProjection(cfg->focal_length, cfg->sensor_width, fb_size);
    if (eye.z < 0.0) projection = glm::scale(projection, glm::vec3(-1));
    auto VP = projection * view;

    af_fbo->bind();

    quad.bind();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    glDisable(GL_CULL_FACE);

    // Discard fragments outside of search region
    glScissor(search_min.x, search_min.y, search_size.x, search_size.y);

    af_shader->use();

    glUniform3fv(af_shader->getLocation("eye"), 1, &eye[0]);
    glUniform1f(af_shader->getLocation("focus_distance"), cfg->focus_distance);
    glUniform1f(af_shader->getLocation("size"), 3.0f * std::max(camera_array->uv_size.x, camera_array->uv_size.y));
    glUniform3fv(af_shader->getLocation("forward"), 1, &forward[0]);
    glUniformMatrix4fv(af_shader->getLocation("VP"), 1, GL_FALSE, &VP[0][0]);

    for (int i = 0; i < 2; i++)
    {
        if (!camera_array->light_slab)
        {
            camera_array->bind(cameras[i], af_shader->getLocation("data_eye"), af_shader->getLocation("data_VP"));
        }
        else
        {
            camera_array->bind(cameras[i], af_shader->getLocation("data_eye"));
            glm::vec2 st_size(camera_array->cameras[i].size);
            st_size = (st_size / st_size.x) * (float)cfg->st_width;
            glUniform2fv(af_shader->getLocation("st_size"), 1, &st_size[0]);
            glUniform1f(af_shader->getLocation("st_distance"), cfg->st_distance);
        }
        glUniform1i(af_shader->getLocation("channel"), i);
        quad.draw();
    }
    
    af_fbo->unBind();

    fbo->bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    af_fbo->bindTexture();

    // Discard fragments outside of search region
    glScissor(search_min.x, search_min.y, search_size.x, search_size.y);

    template_match_shader.use();

    glUniform2iv(template_match_shader.getLocation("size"), 1, &af_fbo->size[0]);
    glUniform2iv(template_match_shader.getLocation("template_min"), 1, &template_min[0]);
    glUniform2iv(template_match_shader.getLocation("template_max"), 1, &template_max[0]);

    quad.draw();

    float min_diff = std::numeric_limits<float>::max();
    glm::vec2 best(-1.0f);

    std::vector<float> data(search_size.x * search_size.y);
    glReadPixels(search_min.x, search_min.y, search_size.x, search_size.y, GL_RED, GL_FLOAT, data.data());
    fbo->unBind();

    for (int x = 0; x < search_size.x; x++)
    {
        for (int y = 0; y < search_size.y; y++)
        {
            float diff = data[x + y * search_size.y];
            if (diff < min_diff)
            {
                min_diff = diff;
                best = { x, y };
            }
        }
    }

    best += glm::vec2(af_size) / 2.0f;

    glm::vec2 pixel_disparity = (glm::vec2(search_size) / 2.0f) - best;
    //glm::vec2 uv_disparity = pixel_disparity / glm::vec2(fb_size);

    //fbo->bind();
    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    //glClear(GL_COLOR_BUFFER_BIT);
    //debug_shader.use();
    //af_fbo->bindTexture();
    //glUniform2fv(debug_shader.getLocation("disparity"), 1, &uv_disparity[0]);
    //quad.draw();
    //fbo->unBind();

    glm::vec3 up(view[0][1], view[1][1], view[2][1]);

    auto pixelToFocalPlane = [&](const glm::vec2 &px)
    {
        float x = (float)cfg->sensor_width * ((px.x - (fb_size.x / 2.0f)) / fb_size.x);
        float y = (float)cfg->sensor_width * ((px.y - (fb_size.y / 2.0f)) / fb_size.x);

        glm::vec3 direction = glm::normalize(right * x + up * y + forward * (float)cfg->focal_length);
        return eye + direction * ((float)cfg->focus_distance / glm::dot(direction, forward));
    };

    // Pixels projected to focal plane
    glm::vec3 f0 = pixelToFocalPlane(glm::vec2(af_pos));
    glm::vec3 f1 = pixelToFocalPlane(glm::vec2(af_pos) + pixel_disparity);

    // Camera positions
    glm::vec3 c0 = glm::vec3(camera_array->cameras[cameras.x].uv, 0.0f);
    glm::vec3 c1 = glm::vec3(camera_array->cameras[cameras.y].uv, 0.0f);

    // Directions from cameras to points on focal plane
    glm::vec3 d0 = glm::normalize(f0 - c0);
    glm::vec3 d1 = glm::normalize(f1 - c1);
    //glm::vec3 d0 = f0 - c0;
    //glm::vec3 d1 = f1 - c1;

    glm::vec2 best_ab;
    float min_distance = 10000.0f;
    for (float a = 0.0f; a < 3.0f; a += 0.01f)
    {
        for (float b = 0.0f; b < 3.0f; b += 0.01f)
        {
            glm::vec3 p0 = c0 + d0 * a;
            glm::vec3 p1 = c1 + d1 * b;
            float distance = glm::distance(p0, p1);
            if (distance < min_distance)
            {
                min_distance = distance;
                best_ab = { a, b };
            }
        }
    }

    //glm::vec3 a = c0;
    //glm::vec3 b = d0;
    //glm::vec3 c = c1;
    //glm::vec3 d = d1;

    //float s = (glm::dot(b, d) * (glm::dot(a, d) - glm::dot(b, c)) - glm::dot(a, d) * glm::dot(c, d)) / (std::pow(glm::dot(b, d), 2) - 1.0);
    //float t = (glm::dot(b, d) * (glm::dot(c, d) - glm::dot(a, d)) - glm::dot(b, c) * glm::dot(a, b)) / (std::pow(glm::dot(b, d), 2) - 1.0);

    //glm::vec3 r = 0.5f * (a + c + b * t + d * s);

    glm::vec3 r = (c0 + d0 * best_ab.x + c1 + d1 * best_ab.y) * 0.5f;

    std::cout << glm::distance(eye, r) << std::endl;

    cfg->focus_distance = glm::distance(eye, r);

    //std::cout << disparity.x << " " << disparity.y << ", " << uv_disparity.x << " " << uv_disparity.y << std::endl;
}

float getContrast()
{
    auto gammaExpand = [](glm::vec3 c)
    {
        for (uint8_t i = 0; i < 3; i++) 
            c[i] = c[i] <= 0.04045f ? c[i] / 12.92f : std::pow((c[i] + 0.055f) / 1.055f, 2.4f);

        return c;
    };

    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    glm::ivec2 res(64);
    std::vector<glm::vec4> data(res.x * res.y);
    glReadPixels(vp[0] + vp[2] / 2 - res.x / 2, vp[1] + vp[3] / 2 - res.y / 2, res.x, res.y, GL_RGBA, GL_FLOAT, data.data());

    float max_l = -1.0f;
    float min_l = std::numeric_limits<float>::max();

    float total_l = 0.0f;

    for (int y = 0; y < res.y; y++)
    {
        for (int x = 0; x < res.x; x++)
        {
            //auto v = glm::vec3(
            //    data[y * res.y + x + 0],
            //    data[y * res.y + x + 1],
            //    data[y * res.y + x + 2]
            //) / 255.0f;
            auto v = glm::vec3(data[y * res.y + x]);

            v = gammaExpand(v);

            float l = 0.2126f * v.r + 0.7152 * v.g + 0.0722 * v.b;

            if (max_l < l) max_l = l;
            if (min_l > l) min_l = l;

            total_l += l;
        }
    }

    float contrast = (max_l - min_l) / (total_l / (res.x * res.y));

    return std::isfinite(contrast) ? std::sqrt(contrast) : 0.0f;
}