#pragma once

inline constexpr char *normalize_aperture_weights_frag = R"glsl(
#version 330 core
#line 5

uniform sampler2D accumulation_texture;

in vec2 interpolated_texcoord;

out vec4 color;

vec3 gammaCompress(vec3 c)
{
    return mix(
        1.055 * pow(c, vec3(1.0/2.4)) - 0.055,
        c * 12.92,
        lessThan(c, vec3(0.0031308))
    );
}

void main()
{
    vec4 c = texture(accumulation_texture, interpolated_texcoord);
    color.xyz = gammaCompress(c.xyz / max(1.0, c.w));
})glsl";