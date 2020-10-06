#pragma once

inline constexpr char *draw_disparity_frag = R"glsl(
#version 330 core
#line 5

uniform sampler2D disparity_image;

in vec2 interpolated_texcoord;

out vec4 color;

vec2 gammaCompress(vec2 c)
{
    return mix(
        1.055 * pow(c, vec2(1.0/2.4)) - 0.055,
        c * 12.92,
        lessThan(c, vec2(0.0031308))
    );
}

void main()
{
    vec2 c = gammaCompress(texture(disparity_image, interpolated_texcoord).xy);
    color = vec4(c.r, 0.5 * (c.r + c.g), c.g, 1.0);
})glsl";