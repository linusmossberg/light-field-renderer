#pragma once

class MyShader
{
public:
    MyShader(const char* vert_source, const char* frag_source);

    int getLocation(const char* name);

    void use();

    int handle;
};