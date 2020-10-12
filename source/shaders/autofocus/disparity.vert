#pragma once

inline constexpr char *disparity_vert = R"(
#version 330 core
#line 5

// Properties of desired camera
uniform float focus_distance;
uniform vec3 eye;
uniform vec3 forward;
uniform mat4 VP;

// Properties of current data camera
uniform vec2 data_eye;

uniform vec2 size;

layout (location = 0) in vec2 position;

out vec2 st;

/******************************************************************
Forward declared fuction that is appended later depending on the 
data camera projection/parameterization used for the light field.
******************************************************************/
vec2 projectToDataCamera(vec3 point);

void main() 
{
    vec2 camera_plane = position * size;

    vec3 direction = normalize(vec3(camera_plane, 0.0) - eye);
    vec3 focal_point = eye + direction * (focus_distance / dot(direction, forward));

    st = projectToDataCamera(focal_point);

    gl_Position = VP * vec4(camera_plane, 0.0, 1.0);
})";