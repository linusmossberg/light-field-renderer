#include "config.hpp"

#include <fstream>
#include <sstream>

void Config::open(std::filesystem::path path)
{
    folder = path.parent_path().string();

    defaults();

    if (path.extension() != ".cfg")
    {
        path = std::filesystem::path(folder) / "config.cfg";
    }

    if (std::filesystem::exists(path))
    {
        std::ifstream file(path);

        std::string line;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::string name;
            ss >> name;

            auto readProperty = [&](auto &p)
            {
                float value, min, max;
                ss >> value;
                ss >> min;
                ss >> max;

                p = Property(value, min, max, p.getScale());

                if (!ss.eof() || ss.fail() || !p.valid())
                {
                    throw std::runtime_error("Invalid config file: " + name);
                }
            };

            if (properties.find(name) != properties.end())
            {
                readProperty(*properties[name]);
            }
        }
    }
}

void Config::registerProperty(const std::string &name, Property* prop_ptr, const Property &prop_val)
{
    properties[name] = prop_ptr;
    *prop_ptr = prop_val;
}

void Config::defaults()
{
    registerProperty("focal-length", &focal_length, Property(50.0f, 10.0f, 100.0f, 1e-3f));
    registerProperty("sensor-width", &sensor_width, Property(36.0f, 10.0f, 100.0f, 1e-3f));
    registerProperty("f-stop", &f_stop, Property(1.0f, 0.4f, 5.6f));
    registerProperty("focus-distance", &focus_distance, Property(1.0f, 0.5f, 5.0f));
    registerProperty("st-width", &st_width, Property(1.0f, 0.1f, 2.0f));
    registerProperty("st-distance", &st_distance, Property(1.0f, 0.1f, 2.0f));
    registerProperty("aperture-falloff", &aperture_falloff, Property(1.0f, 0.01f, 1.99f));

    registerProperty("target-x", &target_x, Property(0.0f, -5.0f, 5.0f));
    registerProperty("target-y", &target_y, Property(0.0f, -5.0f, 5.0f));
    registerProperty("target-z", &target_z, Property(-3.0f, -10.0f, 0.0f));

    registerProperty("speed", &speed, Property(0.2f, 0.01f, 1.0f));

    registerProperty("x", &x, Property(0.0f, -3.0f, 3.0f));
    registerProperty("y", &y, Property(0.0f, -3.0f, 3.0f));
    registerProperty("z", &z, Property(0.2f, -3.0f, 3.0f));

    registerProperty("animation-sway", &animation_sway, Property(1.0f, -5.0f, 5.0f));
    registerProperty("animation-depth", &animation_depth, Property(2.0f, 0.1f, 10.0f));
    registerProperty("animation-cycles", &animation_cycles, Property(3.0f, 1.0f, 10.0f));
    registerProperty("animation-scale", &animation_scale, Property(1.0f, 0.1f, 2.0f));
    registerProperty("animation-duration", &animation_duration, Property(4.0f, 1.0f, 12.0f));

    registerProperty("autofocus-x", &autofocus_x, Property(0.5f, 0.0f, 1.0f));
    registerProperty("autofocus-y", &autofocus_y, Property(0.5f, 0.0f, 1.0f));

    registerProperty("template-size", &template_size, Property(64.0f, 16.0f, 128.0f));
    registerProperty("search-scale", &search_scale, Property(2.0f, 1.5f, 4.0f));

    registerProperty("width", &width, Property(512.0f, 256.0f, 16384.0f));
    registerProperty("height", &height, Property(512.0f, 256.0f, 16384.0f));
    registerProperty("exposure", &exposure, Property(0.0f, -1.0f, 1.0f));

    registerProperty("pitch", &pitch, Property(0.0f, -89.9f, 89.9f, glm::radians(1.0f)));
    registerProperty("yaw", &yaw, Property(0.0f, -89.9f, 89.9f, glm::radians(1.0f)));
}