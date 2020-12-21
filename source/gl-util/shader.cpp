#include "shader.hpp"

#include <exception>
#include <stdexcept>

#include <nanogui/opengl.h>

Shader::Shader(const char* vert_source, const char* frag_source)
{
    int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vert_source, NULL);
    glCompileShader(vertex_shader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
        throw std::runtime_error("Vertex shader error: " + std::string(infoLog));
    }

    int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &frag_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
        throw std::runtime_error("Fragment shader error: " + std::string(infoLog));
    }

    handle = glCreateProgram();
    glAttachShader(handle, vertex_shader);
    glAttachShader(handle, fragment_shader);
    glLinkProgram(handle);

    glGetProgramiv(handle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(handle, 512, NULL, infoLog);
        throw std::runtime_error("Shader program error: " + std::string(infoLog));
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    use();
}

Shader::~Shader()
{
    glDeleteProgram(handle);
}

int Shader::getLocation(const char* name)
{
    GLint loc = glGetUniformLocation(handle, name);

    //if (loc < 0) std::cout << name << " uniform location not found.\n";

    return loc;
}

void Shader::use()
{
    glUseProgram(handle);
}