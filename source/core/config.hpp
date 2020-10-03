#pragma once

#include <filesystem>
#include <glm/glm.hpp>

class Config
{
public:
    Config() { defaults(); }

    void open(std::filesystem::path path);

    void defaults();

    struct Property
    {
        Property() { }

        Property(float value, float min, float max, float scale = 1.0f)
            : value(value * scale), min(min * scale), max(max * scale), range((max - min) * scale), scale(scale) { }

        operator float() const { return value; }

        void operator=(const float &v)
        {
            if (v > max) value = max;
            else if (v < min) value = min;
            else value = v;
        }

        void operator+=(const float &v) { *this = value + v; }
        void operator-=(const float &v) { *this = value - v; }

        float getRange() { return range; }
        float getNormalized() { return (value - min) / range; }
        float getDisplay() { return value / scale; }

        bool valid() { return min <= value && max >= value; }

        void setNormalized(float v) { *this = min + v * range; }
        void setDisplay(float v) { *this = v * scale; }

    private:
        float value, min, max, range, scale;
    };

    Property focal_length;
    Property sensor_width;
    Property f_stop;
    Property focus_distance;
    Property aperture_falloff;
    Property st_width;
    Property st_distance;

    Property target_x;
    Property target_y;
    Property target_z;

    Property x;
    Property y;
    Property z;

    Property pitch;
    Property yaw;

    std::string folder;
};