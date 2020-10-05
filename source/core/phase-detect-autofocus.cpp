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

void LightFieldRenderer::phaseDetectAutoFocus()
{
    glm::ivec2 af_pos(glm::vec2(cfg->autofocus_x, cfg->autofocus_y) * glm::vec2(af_fbo->size));

    auto desired_uv = glm::vec2(pixelToFocalPlane(af_pos));
    glm::ivec2 cameras = camera_array->findBestCameras(desired_uv, 0.2f);

    const glm::ivec2 af_size(64);
    const glm::ivec2 search_size(128);

    glm::ivec2 template_min(af_pos - af_size / 2);
    glm::ivec2 template_max(af_pos + af_size / 2);
    glm::ivec2 search_min(af_pos - search_size / 2);
    glm::ivec2 search_max(af_pos + search_size / 2);

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
    glm::dvec2 best;

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

    best += glm::dvec2(af_size) / 2.0;

    glm::dvec2 pixel_disparity = (glm::dvec2(search_size) / 2.0) - best;

    //glm::vec2 uv_disparity = pixel_disparity / glm::vec2(fb_size);

    //fbo->bind();
    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    //glClear(GL_COLOR_BUFFER_BIT);
    //debug_shader.use();
    //af_fbo->bindTexture();
    //glUniform2fv(debug_shader.getLocation("disparity"), 1, &uv_disparity[0]);
    //quad.draw();
    //fbo->unBind();

    // Pixels projected to focal plane
    glm::vec3 f0 = pixelToFocalPlane(glm::dvec2(af_pos));
    glm::vec3 f1 = pixelToFocalPlane(glm::dvec2(af_pos) + pixel_disparity);

    // Camera positions
    glm::vec3 c0 = glm::vec3(camera_array->cameras[cameras.x].uv, 0.0);
    glm::vec3 c1 = glm::vec3(camera_array->cameras[cameras.y].uv, 0.0);

    // Directions from cameras to points on focal plane
    glm::vec3 d0 = glm::normalize(f0 - c0);
    glm::vec3 d1 = glm::normalize(f1 - c1);

    glm::vec3 r = closestPointBetweenRays(c0, d0, c1, d1);

    cfg->focus_distance = glm::distance(eye, r);
}

glm::vec3 LightFieldRenderer::pixelToFocalPlane(const glm::vec2 &px)
{
    float x = (float)cfg->sensor_width * ((px.x - (fb_size.x * 0.5f)) / fb_size.x);
    float y = (float)cfg->sensor_width * ((px.y - (fb_size.y * 0.5f)) / fb_size.x);

    glm::vec3 direction = glm::normalize(right * x + up * y + forward * (float)cfg->focal_length);
    return eye + direction * ((float)cfg->focus_distance / glm::dot(direction, forward));
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