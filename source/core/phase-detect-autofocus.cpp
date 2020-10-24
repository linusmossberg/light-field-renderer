#include "light-field-renderer.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "config.hpp"
#include "camera-array.hpp"
#include "../gl-util/fbo.hpp"
#include "util.hpp"

#include <iostream>

glm::vec3 closestPointBetweenRays(const glm::vec3 &p0, const glm::vec3 &d0, const glm::vec3 &p1, const glm::vec3 &d1);

void LightFieldRenderer::phaseDetectAutofocus()
{
    const glm::ivec2 template_size((int)std::round(cfg->template_size));
    const glm::ivec2 search_size((int)std::round(cfg->template_size * cfg->search_scale));

    glm::ivec2 af_pos(glm::vec2(cfg->autofocus_x, cfg->autofocus_y) * glm::vec2(fb_size));

    glm::clamp(af_pos, search_size / 2 + 1, fb_size - (search_size / 2 + 1));

    cfg->autofocus_x = af_pos.x / (float)fb_size.x;
    cfg->autofocus_y = af_pos.y / (float)fb_size.y;

    glm::ivec2 cameras;
    cameras.x = camera_array->findClosestCamera(pixelToCameraPlane(glm::vec2(fb_size) * 0.4f));
    cameras.y = camera_array->findClosestCamera(pixelToCameraPlane(glm::vec2(fb_size) * 0.6f), cameras.x);

    glm::ivec2 template_min(af_pos - template_size / 2);
    glm::ivec2 template_max(af_pos + template_size / 2);

    glm::ivec2 search_min(af_pos - search_size / 2);
    glm::ivec2 search_max(af_pos + search_size / 2);

    fbo1->bind();

    quad.bind();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    // Discard fragments outside of search region
    if(!visualize_autofocus)
        glScissor(search_min.x, search_min.y, search_size.x, search_size.y);

    disparity_shader->use();

    // Size of visible part of focal plane
    glm::vec2 focal_plane_size = (glm::vec2(fb_size) / (float)fb_size.x) * (cfg->sensor_width / image_distance) * (float)cfg->focus_distance;

    glUniformMatrix4fv(disparity_shader->getLocation("VP"), 1, GL_FALSE, &VP[0][0]);
    glUniform3fv(disparity_shader->getLocation("eye"), 1, &eye[0]);
    glUniform1f(disparity_shader->getLocation("focus_distance"), cfg->focus_distance);
    glUniform2fv(disparity_shader->getLocation("size"), 1, &focal_plane_size[0]);
    glUniform3fv(disparity_shader->getLocation("forward"), 1, &forward[0]);
    glUniform3fv(disparity_shader->getLocation("right"), 1, &right[0]);
    glUniform3fv(disparity_shader->getLocation("up"), 1, &up[0]);

    for (int i = 0; i < 2; i++)
    {
        if (!camera_array->light_slab)
        {
            camera_array->bind(cameras[i], disparity_shader->getLocation("data_eye"), disparity_shader->getLocation("data_VP"));
        }
        else
        {
            camera_array->bind(cameras[i], disparity_shader->getLocation("data_eye"));
            glm::vec2 st_size(camera_array->cameras[i].size);
            st_size = (st_size / st_size.x) * (float)cfg->st_width;
            glUniform2fv(disparity_shader->getLocation("st_size"), 1, &st_size[0]);
            glUniform1f(disparity_shader->getLocation("st_distance"), cfg->st_distance);
        }
        glUniform1i(disparity_shader->getLocation("channel"), i);
        quad.draw();
    }
    
    fbo1->unBind();

    if (visualize_autofocus)
    {
        visualize_autofocus_shader.use();

        glUniform2iv(visualize_autofocus_shader.getLocation("size"), 1, &fb_size[0]);
        glUniform2iv(visualize_autofocus_shader.getLocation("template_min"), 1, &template_min[0]);
        glUniform2iv(visualize_autofocus_shader.getLocation("template_max"), 1, &template_max[0]);
        glUniform2iv(visualize_autofocus_shader.getLocation("search_min"), 1, &search_min[0]);
        glUniform2iv(visualize_autofocus_shader.getLocation("search_max"), 1, &search_max[0]);

        if(!(continuous_autofocus || autofocus_click)) return;
    }

    fbo0->bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    fbo1->bindTexture();

    // Discard fragments outside of search region
    glScissor(search_min.x, search_min.y, search_size.x, search_size.y);

    template_match_shader.use();

    glUniform2iv(template_match_shader.getLocation("size"), 1, &fb_size[0]);
    glUniform2iv(template_match_shader.getLocation("template_min"), 1, &template_min[0]);
    glUniform2iv(template_match_shader.getLocation("template_max"), 1, &template_max[0]);

    quad.draw();

    if (sqdiff_data.size() != search_size.x * search_size.y)
    {
        sqdiff_data.resize(search_size.x * search_size.y);
    }
    glReadPixels(search_min.x, search_min.y, search_size.x, search_size.y, GL_RED, GL_FLOAT, sqdiff_data.data());
    fbo0->unBind();

    glm::vec2 best;
    float min_diff = std::numeric_limits<float>::max();
    for (int x = 0; x < search_size.x; x++)
    {
        for (int y = 0; y < search_size.y; y++)
        {
            float diff = sqdiff_data[x + y * search_size.y];
            if (diff < min_diff)
            {
                min_diff = diff;
                best = { x, y };
            }
        }
    }

    best += glm::vec2(template_size) * 0.5f;

    glm::vec2 pixel_phase_difference = (glm::vec2(search_size) * 0.5f) - best;

    // Pixels projected to focal plane
    glm::vec3 f0 = pixelToFocalPlane(glm::vec2(af_pos));
    glm::vec3 f1 = pixelToFocalPlane(glm::vec2(af_pos) + pixel_phase_difference);

    // Camera positions
    glm::vec3 c0 = glm::vec3(camera_array->cameras[cameras.x].xy, 0.0f);
    glm::vec3 c1 = glm::vec3(camera_array->cameras[cameras.y].xy, 0.0f);

    // Directions from cameras to points on focal plane
    glm::vec3 d0 = glm::normalize(f0 - c0);
    glm::vec3 d1 = glm::normalize(f1 - c1);

    // Point on new focal plane
    glm::vec3 nf = closestPointBetweenRays(c0, d0, c1, d1);

    // dot(nf-eye, forward) = |nf-eye|*cos(theta)
    cfg->focus_distance = dot(nf - eye, forward);
}

glm::vec3 LightFieldRenderer::pixelDirection(const glm::vec2 &px)
{
    float x = (float)cfg->sensor_width * ((px.x - (fb_size.x * 0.5f)) / fb_size.x);
    float y = (float)cfg->sensor_width * ((px.y - (fb_size.y * 0.5f)) / fb_size.x);

    return glm::normalize(right * x + up * y + forward * image_distance);
}

glm::vec3 LightFieldRenderer::pixelToFocalPlane(const glm::vec2 &px)
{
    glm::vec3 direction = pixelDirection(px);
    return eye + direction * ((float)cfg->focus_distance / glm::dot(direction, forward));
}

glm::vec2 LightFieldRenderer::pixelToCameraPlane(const glm::vec2 &px)
{
    glm::vec3 direction = pixelDirection(px);
    return glm::vec2(eye) + glm::vec2(direction) * (-eye.z / direction.z);
}

// ray0(s) = p0 + d0 * s
// ray1(t) = p1 + d1 * t
// http://wiki.unity3d.com/index.php/3d_Math_functions?_ga=2.21692155.1539381585.1601871902-499235835.1601871902
glm::vec3 closestPointBetweenRays(const glm::vec3 &p0, const glm::vec3 &d0, const glm::vec3 &p1, const glm::vec3 &d1)
{
    float a = glm::dot(d0, d0);
    float b = glm::dot(d0, d1);
    float e = glm::dot(d1, d1);

    float d = a * e - b * b;

    // Lines are parallel
    if (d == 0.0f) return 0.5f * (p0 + p1);;

    glm::vec3 r = p0 - p1;
    float c = glm::dot(d0, r);
    float f = glm::dot(d1, r);

    float s = (b*f - c * e) / d;
    float t = (a*f - c * b) / d;

    return 0.5f * ((p0 + d0 * s) + (p1 + d1 * t));
};