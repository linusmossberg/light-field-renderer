#pragma once

inline constexpr char *phase_vert = R"(
#version 330 core
#line 5

// Properties of desired camera
uniform float focus_distance;
uniform vec3 eye;
uniform vec3 forward;
uniform float size;
uniform mat4 VP;

// Properties of current data camera
uniform vec2 data_eye;

uniform vec2 center_pos;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;

out vec2 uv;
out vec2 st;

vec3 projectToFocalPlane(vec2 point)
{
    vec3 direction = normalize(vec3(point, 0.0) - eye);
    return eye + direction * (focus_distance / dot(direction, forward));
}

/******************************************************************
Forward declared fuction that is appended later depending on the 
data camera projection/parameterization used for the light field.
******************************************************************/
vec2 projectToDataCamera(vec3 point);

void main() 
{
    vec2 p = center_pos + position * size;

    st = projectToDataCamera(projectToFocalPlane(p));
    uv = texcoord;

    gl_Position = VP * vec4(p, 0.0, 1.0);
})";