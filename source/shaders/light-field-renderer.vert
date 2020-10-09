#pragma once

inline constexpr char *light_field_renderer_vert = R"(
#version 330 core
#line 5

// Properties of desired camera
uniform mat4 VP;
uniform float focus_distance;
uniform vec3 eye;
uniform vec3 forward;
uniform vec3 right;
uniform vec3 up;
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

vec2 cameraPlaneAperture()
{
    // Subtract because the vertex will be located on the opposite side of the data eye
    vec3 aperture = eye - (position.x * right + position.y * up) * aperture_diameter;

    // Project aperture point to focal plane through the ray that passes through the data camera
    vec3 a2d = normalize(vec3(data_eye, 0.0) - aperture);
    vec3 focal_point = aperture + a2d * (focus_distance / dot(a2d, forward));

    // Project eye point to to camera plane through the ray that passes through this focal point
    vec3 e2f = normalize(focal_point - eye);
    return eye.xy + e2f.xy * (-eye.z / e2f.z);
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