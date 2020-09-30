#pragma once

inline constexpr char *light_field_renderer_vert = R"(
#version 330 core
#line 5

// Properties of desired camera
uniform mat4 VP;
uniform float focus_distance;
uniform vec3 eye;
uniform vec3 forward;
uniform float aperture_diameter;

// Properties of current data camera
uniform vec2 data_eye;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;

out vec2 uv;
out vec2 st;

vec3 projectToFocalPlane(vec2 point)
{
    vec3 direction = normalize(vec3(point, 0.0) - eye);
    return eye + direction * (focus_distance / dot(direction, forward));
}

/****************************************************************************
                  focal plane
                 ------o------
               ^      /|\
               |     / | \
focus distance |    /  |  \
               |   x---d---x    d = data eye, x = new aperture vertices
               v  /    |    \
                 p-----e-----p  e = desired eye, p = old aperture vertices
                 <-----D----->  D = aperture diameter

****************************************************************************/
vec2 cameraPlaneAperture()
{
    float d = focus_distance - sign(eye.z) * distance(eye, vec3(data_eye, 0.0));
    float slope = aperture_diameter / focus_distance;
    return data_eye + position * (slope * d * forward.z);
}

/******************************************************************
Forward declared fuction that is appended later depending on the 
data camera projection/parameterization used for the light field.
******************************************************************/
vec2 projectToDataCamera(vec3 point);

void main() 
{
    vec2 p = cameraPlaneAperture();

    st = projectToDataCamera(projectToFocalPlane(p));
    uv = texcoord;

    gl_Position = VP * vec4(p, 0.0, 1.0);
})";