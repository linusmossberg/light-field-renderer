#pragma once

#include <glm/glm.hpp>

class FBO
{
public:
    FBO(const glm::ivec2 &size);

    void bind();

    void unBind();

    void bindTexture();

    unsigned int handle, texture;
    const glm::ivec2 size;

    int prev_viewport[4] = { 0 };
    int prev_scissor[4] = { 0 };
};