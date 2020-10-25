#include "camera-array.hpp"

#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "util.hpp"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

CameraArray::CameraArray(const std::filesystem::path& path)
{
    stbi_set_flip_vertically_on_load(true);

    const std::vector<std::string> extensions = {
        "JPEG", "JPG", "PNG", "TGA", "BMP", "PSD", "GIF", "HDR", "PIC", "PNM"
    };

    glm::vec2 max_xy(std::numeric_limits<float>::lowest());
    glm::vec2 min_xy(std::numeric_limits<float>::max());

    light_slab = true;

    for (const auto& file : std::filesystem::directory_iterator(path))
    {
        if (!file.path().has_extension())
        {
            continue;
        }
        else
        {
            std::string ext = file.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), toupper);
            bool is_valid = false;
            for (const auto& valid : extensions)
            {
                if (ext == std::string("." + valid))
                {
                    is_valid = true;
                    break;
                }
            }
            if (!is_valid)
            {
                continue;
            }
        }

        std::filesystem::path extensionless = file.path();
        extensionless.replace_extension("");
        std::stringstream ss(extensionless.filename().string());

        std::vector<std::string> properties;
        while (ss.good())
        {
            std::string p;
            std::getline(ss, p, '_');
            if(!p.empty()) properties.push_back(p);
        }

        // name_i_j_-y_x, with extension _focal-length_sensor-width

        if (properties.size() != 5 && properties.size() != 7) continue;

        if (!cameras.empty())
        {
            if (properties.size() == 5 && !light_slab) continue;
            if (properties.size() == 7 &&  light_slab) continue;
        }
            
        float y = -std::stof(properties[3]) * 1e-3f;
        float x =  std::stof(properties[4]) * 1e-3f;

        float focal_length = -1.0f, sensor_width = -1.0f;
        if (properties.size() == 7)
        {
            focal_length = std::stof(properties[5]);
            sensor_width = std::stof(properties[6]);

            light_slab = false;
        }

        std::cout << "\r" << std::string(96, ' ');
        std::cout << "\rLoading " << file.path().filename();

        int width, height, channels;
        uint8_t* image_data = stbi_load(file.path().string().c_str(), &width, &height, &channels, 0);

        if (!image_data) continue;

        int pixel_format;
        switch (channels)
        {
        case 1: pixel_format = GL_RED; break;
        case 2: pixel_format = GL_RG; break;
        case 3: pixel_format = GL_RGB; break;
        case 4: pixel_format = GL_RGBA; break;
        default: stbi_image_free(image_data); continue;
        }

        // Image is valid
        Camera dc;

        dc.size = { width, height };
        dc.xy = { x, y };
        dc.pixel_format = pixel_format;
        dc.focal_length = focal_length;
        dc.sensor_width = sensor_width;

        glGenTextures(1, &dc.texture);
        glBindTexture(GL_TEXTURE_2D, dc.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, pixel_format, width, height, 0, pixel_format, GL_UNSIGNED_BYTE, image_data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        stbi_image_free(image_data);

        cameras.push_back(dc);

        if (x > max_xy[0]) max_xy[0] = x;
        if (y > max_xy[1]) max_xy[1] = y;
        if (x < min_xy[0]) min_xy[0] = x;
        if (y < min_xy[1]) min_xy[1] = y;
    }

    std::cout << std::endl;

    if (!cameras.empty())
    {
        xy_size = max_xy - min_xy;

        for (size_t i = 0; i < cameras.size(); i++)
        {
            auto& c = cameras[i];

            c.xy -= min_xy;
            c.xy -= xy_size / 2.0f;

            if (!light_slab)
            {
                auto view = glm::lookAt(
                    glm::vec3(c.xy.x, c.xy.y, 0),
                    glm::vec3(c.xy.x, c.xy.y, -1),
                    glm::vec3(0, 1, 0)
                );

                auto projection = perspectiveProjection(c.focal_length, c.sensor_width, c.size);

                c.VP = projection * view;
            }
        }
    }
    else
    {
        throw std::runtime_error("Invalid light field folder, no images were loaded.");
    }
}

CameraArray::~CameraArray()
{
    for (const auto &c : cameras)
    {
        glDeleteTextures(1, &c.texture);
    }
}

void CameraArray::bind(size_t index, int eye_loc, int VP_loc)
{
    const auto& c = cameras.at(index);
    glBindTexture(GL_TEXTURE_2D, c.texture);
    glUniform2fv(eye_loc, 1, &c.xy[0]);

    if (!light_slab)
    {
        glUniformMatrix4fv(VP_loc, 1, GL_FALSE, &c.VP[0][0]);
    }
}

int CameraArray::findClosestCamera(const glm::vec2 &xy, int exclude_idx)
{
    int idx = 0;
    float min_dist = std::numeric_limits<float>::max();

    for (int i = 0; i < cameras.size(); i++)
    {
        if (i == exclude_idx) continue;
        float dist = glm::distance(cameras[i].xy, xy);
        if (dist < min_dist)
        {
            idx = i;
            min_dist = dist;
        }
    }

    return idx;
}
