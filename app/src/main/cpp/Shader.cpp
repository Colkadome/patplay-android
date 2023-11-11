#include "Shader.h"

#include "AndroidOut.h"
#include "Model.h"
#include "Utility.h"

// Vertex shader.
static const char *vertexSource = R"vertex(#version 300 es
in vec2 inPosition;
in vec2 inUV;

out vec2 fragUV;

uniform vec4 uPosSize;
uniform mat4 uProjection;

void main() {
    fragUV = inUV;
    gl_Position = uProjection * vec4((inPosition * uPosSize.zw) + uPosSize.xy, 0.0, 1.0);
}
)vertex";

// Fragment shader.
static const char *fragmentSource = R"fragment(#version 300 es
precision mediump float;

in vec2 fragUV;

uniform sampler2D uTexture;
uniform vec4 uColor;

out vec4 outColor;

void main() {
    outColor = texture(uTexture, fragUV) * uColor;
}
)fragment";

Shader *Shader::loadShader() {
    Shader *shader = nullptr;

    // Load vertex shader.

    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader) {
        return nullptr;
    }

    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return nullptr;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);

        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

            // If we fail to link the shader program, log the result for debugging
            if (logLength) {
                auto *log = new GLchar[logLength];
                glGetProgramInfoLog(program, logLength, nullptr, log);
                aout << "Failed to link program with:\n" << log << std::endl;
                delete[] log;
            }

            glDeleteProgram(program);
        } else {

            // Get the attribute and uniform locations by name. You may also choose to hardcode
            // indices with layout= in your shader, but it is not done in this sample
            GLint positionAttribute = glGetAttribLocation(program, "inPosition");
            GLint uvAttribute = glGetAttribLocation(program, "inUV");
            GLint posSizeUniform = glGetUniformLocation(program, "uPosSize");
            GLint projectionMatrixUniform = glGetUniformLocation(program, "uProjection");
            GLint colorUniform = glGetUniformLocation(program, "uColor");

            // Only create a new shader if all the attributes are found.
            if (positionAttribute != -1
                && uvAttribute != -1
                && projectionMatrixUniform != -1
                && colorUniform != -1) {

                shader = new Shader(
                        program,
                        positionAttribute,
                        uvAttribute,
                        posSizeUniform,
                        projectionMatrixUniform,
                        colorUniform);
            } else {
                glDeleteProgram(program);
            }
        }
    }

    // The shaders are no longer needed once the program is linked. Release their memory.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shader;
}

GLuint Shader::loadShader(GLenum shaderType, const std::string &shaderSource) {
    Utility::assertGlError();
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        auto *shaderRawString = (GLchar *) shaderSource.c_str();
        GLint shaderLength = shaderSource.length();
        glShaderSource(shader, 1, &shaderRawString, &shaderLength);
        glCompileShader(shader);

        GLint shaderCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);

        // If the shader doesn't compile, log the result to the terminal for debugging
        if (!shaderCompiled) {
            GLint infoLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);

            if (infoLength) {
                auto *infoLog = new GLchar[infoLength];
                glGetShaderInfoLog(shader, infoLength, nullptr, infoLog);
                aout << "Failed to compile with:\n" << infoLog << std::endl;
                delete[] infoLog;
            }

            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

void Shader::activate() const {
    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
}

void Shader::setTexture(const unsigned int tex) const {
    glBindTexture(GL_TEXTURE_2D, tex);
}

void Shader::deactivate() const {
    glUseProgram(0);
}

void Shader::drawShape(const float x, const float y, const float w, const float h) const {

    // TODO: Try and use VBOs so we don't need to pass the data each time.
    // https://www.khronos.org/opengl/wiki/Tutorial2:_VAOs,_VBOs,_Vertex_and_Fragment_Shaders_(C_/_SDL)

    // Load vertex data for a quad.
    float position_data[] = {
            0.5, 0.5,
            -0.5, 0.5,
            -0.5, -0.5,
            0.5, 0.5,
            -0.5, -0.5,
            0.5, -0.5
    };
    glVertexAttribPointer(position_,2,GL_FLOAT,GL_FALSE,0,position_data);
    glEnableVertexAttribArray(position_);

    // Load UV data for a quad.
    float uv_data[] = {
            1, 0,
            0, 0,
            0, 1,
            1, 0,
            0, 1,
            1, 1
    };
    glVertexAttribPointer(uv_,2,GL_FLOAT,GL_FALSE,0,uv_data);
    glEnableVertexAttribArray(uv_);

    glUniform4f(posSize_, x, y, w, h);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(uv_);
    glDisableVertexAttribArray(position_);

}

void Shader::setProjectionMatrix(float *projectionMatrix) const {
    glUniformMatrix4fv(projectionMatrix_, 1, false, projectionMatrix);
}

void Shader::setColor(float r, float g, float b, float a) const {
    glUniform4f(color_, r, g, b, a);
}