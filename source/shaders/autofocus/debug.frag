#pragma once

inline constexpr char *debug_frag = R"glsl(
#version 330 core
#line 5

uniform sampler2D disparity_image;

uniform vec2 disparity;

in vec2 interpolated_texcoord;

out vec4 color;

void main()
{
    float v0 = texture(disparity_image, interpolated_texcoord).r;
    float v1 = texture(disparity_image, interpolated_texcoord + disparity).g;
    color = vec4(v0, 0.5 * (v0 + v1), v1, 1.0);
})glsl";