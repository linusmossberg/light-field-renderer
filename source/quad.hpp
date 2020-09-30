#pragma once

class Quad
{
public:
    Quad();

    ~Quad();

    void bind();

    void draw();

    unsigned int VBO, VAO;
};