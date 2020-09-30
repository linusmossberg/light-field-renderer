#pragma once

#include <filesystem>

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

        float get() { return value; }
        float getRange() { return range; }
        float getMin() { return min; }
        float getMax() { return max; }
        float getNormalized() { return (value - min) / range; }
        float getDisplay() { return value / scale; }

        void add(float v) { set(value + v); }
        bool valid() { return min <= value && max >= value; }

        void setNormalized(float v) { set(min + v * range); }

        void set(float v)
        {
            if (v > max) value = max;
            else if (v < min) value = min;
            else value = v;
        }

    private:
        float value, min, max, range, scale;
    };

    Property focal_length;
    Property sensor_width;
    Property f_stop;
    Property focus_distance;
    Property st_width;
    Property st_distance;

    Property x;
    Property y;
    Property z;

    Property pitch;
    Property yaw;

    std::string image_folder;
};