#include "config.hpp"

#include <fstream>
#include <sstream>

#include "constants.hpp"

void Config::open(std::filesystem::path path)
{
    image_folder = path.parent_path().string();

    defaults();

    if (path.extension() != ".cfg")
    {
        path = std::filesystem::path(image_folder) / "config.cfg";
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

            auto readProperty = [&](auto &p, float scale = 1.0f)
            {
                float value, min, max;
                ss >> value;
                ss >> min;
                ss >> max;

                p = Property(value, min, max, scale);

                if (!ss.eof() || ss.fail() || !p.valid())
                {
                    throw std::runtime_error("Invalid config file.");
                }
            };

            if (name == "focal-length")        readProperty(focal_length, 1e-3f);
            else if (name == "sensor-size")    readProperty(sensor_width, 1e-3f);
            else if (name == "f-stop")         readProperty(f_stop);
            else if (name == "focus-distance") readProperty(focus_distance);
            else if (name == "st-width")       readProperty(st_width);
            else if (name == "st-distance")    readProperty(st_distance);
            else if (name == "x")              readProperty(x);
            else if (name == "y")              readProperty(y);
            else if (name == "z")              readProperty(z);
            else if (name == "yaw")            readProperty(yaw);
            else if (name == "pitch")          readProperty(pitch);
        }
    }
}

void Config::defaults()
{
    focal_length = Property(50.0f, 10.0f, 100.0f, 1e-3f);
    sensor_width = Property(36.0f, 10.0f, 100.0f, 1e-3f);
    f_stop = Property(1.0f, 0.4f, 5.6f);
    focus_distance = Property(1.0f, 0.5f, 5.0f);
    st_width = Property(1.0f, 0.1f, 2.0f);
    st_distance = Property(1.0f, 0.1f, 2.0f);

    x = Property(0.0f, -3.0f, 3.0f);
    y = Property(0.0f, -3.0f, 3.0f);
    z = Property(0.2f, -3.0f, 3.0f);

    pitch = Property(0.0f, -89.9f, 89.9f, C::PI / 180.0f);
    yaw = Property(0.0f, -89.9f, 89.9f, C::PI / 180.0f);
}