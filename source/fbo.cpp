#include "fbo.hpp"

#include <iostream>
#include <exception>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

FBO::FBO(const glm::ivec2 &size) : size(size)
{
    glGenFramebuffers(1, &handle);
    glBindFramebuffer(GL_FRAMEBUFFER, handle);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Framebuffer not complete.");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::bind()
{
    glGetIntegerv(GL_VIEWPORT, prev_viewport);
    glViewport(0, 0, size.x, size.y);

    glGetIntegerv(GL_SCISSOR_BOX, prev_scissor);
    glScissor(0, 0, size.x, size.y);

    // Not sure if necessary since I don't attach stencil or depth buffers to FBO
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, handle);
}

void FBO::unBind()
{
    glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    glScissor(prev_scissor[0], prev_scissor[1], prev_scissor[2], prev_scissor[3]);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::bindTexture()
{
    glBindTexture(GL_TEXTURE_2D, texture);
}