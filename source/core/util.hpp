#pragma once

#include <glm/glm.hpp>

inline std::ostream& operator<<(std::ostream& out, const glm::dvec3& v)
{
    return out << std::string("( " + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + " )");
}

// Perspective projection without near and far clipping
inline glm::mat4 perspectiveProjection(float focal_length, float sensor_width, glm::ivec2 size)
{   
    glm::mat4 P(0.0f);
    P[0][0] = (2.0f * focal_length) / sensor_width;
    P[1][1] = (2.0f * focal_length * size.x) / (sensor_width * size.y);
    P[2][3] = -1.0f;
    return  P;
}
