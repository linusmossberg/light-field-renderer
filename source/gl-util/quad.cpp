#include "quad.hpp"

#include <nanogui/opengl.h>

Quad::Quad()
{
    constexpr float vertices[] =
    {
        // xy         // st
       -0.5f, -0.5f,  0.0f, 0.0f,
        0.5f, -0.5f,  1.0f, 0.0f,
        0.5f,  0.5f,  1.0f, 1.0f,

       -0.5f, -0.5f,  0.0f, 0.0f,
        0.5f,  0.5f,  1.0f, 1.0f,
       -0.5f,  0.5f,  0.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

Quad::~Quad()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Quad::bind()
{
    glBindVertexArray(VAO);
}

void Quad::draw()
{
    glDrawArrays(GL_TRIANGLES, 0, 6);
}