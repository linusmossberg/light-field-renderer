#pragma once

class Shader
{
public:
    Shader(const char* vert_source, const char* frag_source);

    ~Shader();

    int getLocation(const char* name);

    void use();

    int handle;
};