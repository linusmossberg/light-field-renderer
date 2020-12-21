#pragma once

#include <filesystem>
#include <vector>

#include <glm/glm.hpp>

class CameraArray
{
public:
    CameraArray(const std::filesystem::path& path);
    ~CameraArray();

    void bind(size_t index, int eye_loc, int VP_loc, int st_size_loc, int st_distance_loc, float st_width, float st_distance);

    struct Camera
    {
        glm::ivec2 size;
        glm::vec2 xy;
        glm::uvec2 ij;
        int pixel_format;
        unsigned int texture;

        float focal_length;
        float sensor_width;
        glm::mat4 VP;
    };

    bool light_slab;

    int findClosestCamera(const glm::vec2 &xy, int exclude_idx = -1);

    glm::vec2 xy_size;

    std::vector<Camera> cameras;
};
