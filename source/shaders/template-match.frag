#pragma once

inline constexpr char *template_match_frag = R"glsl(
#version 330 core
#line 5

uniform sampler2D disparity_image;

uniform ivec2 size;
uniform ivec2 template_min;
uniform ivec2 template_max;

in vec2 interpolated_texcoord;

out vec4 color;

void main()
{
    vec2 I_xy = interpolated_texcoord * size;

    float sum = 0.0;
    for(int x = template_min.x; x < template_max.x; x++)
    {
        for(int y = template_min.y; y < template_max.y; y++)
        {
            vec2 T_xy = vec2(x, y);

            float I = texture(disparity_image, (I_xy + T_xy - template_min) / size).r;
            float T = texture(disparity_image, T_xy / size).g;

            sum += pow(I-T, 2);
        }
    }
    color = vec4(vec3(sum), 1.0);
})glsl";