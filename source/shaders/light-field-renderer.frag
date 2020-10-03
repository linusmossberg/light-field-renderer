#pragma once

inline constexpr char *light_field_renderer_frag = R"(
#version 330 core
#line 5

uniform sampler2D image;

uniform float aperture_falloff;

out vec4 color;

in vec2 uv;
in vec2 st;

vec3 srgbGammaExpand(vec3 c)
{
    return mix(
        pow((c + 0.055) / 1.055, vec3(2.4)), 
        c / 12.92,
        lessThan(c, vec3(0.04045))
    );
}

void main() 
{
    if(st.x < 0 || st.x > 1 || st.y < 0 || st.y > 1)
    {
        discard;
    }

    float aperture_filter = pow(clamp(1.0 - length((uv - 0.5) * 2.0), 0, 1), aperture_falloff);

    color = vec4(srgbGammaExpand(texture(image, st).xyz) * aperture_filter, aperture_filter);
})";