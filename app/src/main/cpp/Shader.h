#ifndef ANDROIDGLINVESTIGATIONS_SHADER_H
#define ANDROIDGLINVESTIGATIONS_SHADER_H

#include <string>
#include <GLES3/gl3.h>

class Model;

/*!
 * A class representing a simple shader program. It consists of vertex and fragment components. The
 * input attributes are a position (as a Vector3) and a uv (as a Vector2). It also takes a uniform
 * to be used as the entire model/view/projection matrix. The shader expects a single texture for
 * fragment shading, and does no other lighting calculations (thus no uniforms for lights or normal
 * attributes).
 */
class Shader {
public:
    /*!
     * Loads a shader.
     * @return a valid Shader on success, otherwise null.
     */
    static Shader *loadShader();

    inline ~Shader() {
        deactivate();  // We deactivate as there is only one shader.
        if (program_) {
            glDeleteProgram(program_);
            program_ = 0;
        }
        if (vao_) {
            glDeleteVertexArrays(1, &vao_);
            vao_ = 0;
        }
    }

    /*!
     * Prepares the shader for use, call this before executing any draw commands
     */
    void activate() const;

    /*!
     * Cleans up the shader after use, call this after executing any draw commands
     */
    void deactivate() const;

    /*!
     * Sets the current texture
     */
    void setTexture(const unsigned int tex) const;

    /*!
     * Renders a single shape
     */
    void drawShape(const float x, const float y, const float w, const float h) const;

    /*!
     * Sets the model/view/projection matrix in the shader.
     * @param projectionMatrix sixteen floats, column major, defining an OpenGL projection matrix.
     */
    void setProjectionMatrix(float *projectionMatrix) const;

    /*!
     * Sets the current color.
     */
    void setColor(float r, float g, float b, float a) const;

private:
    /*!
     * Helper function to load a shader of a given type
     * @param shaderType The OpenGL shader type. Should either be GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
     * @param shaderSource The full source of the shader
     * @return the id of the shader, as returned by glCreateShader, or 0 in the case of an error
     */
    static GLuint loadShader(GLenum shaderType, const std::string &shaderSource);

    /*!
     * Constructs a new instance of a shader. Use @a loadShader
     * @param program the GL program id of the shader
     * @param position the attribute location of the position
     * @param uv the attribute location of the uv coordinates
     * @param projectionMatrix the uniform location of the projection matrix
     */
    constexpr Shader(
            GLuint program,
            GLint position,
            GLint uv,
            GLint posSize,
            GLint projectionMatrix,
            GLint color,
            GLuint vao)
            : program_(program),
              position_(position),
              posSize_(posSize),
              uv_(uv),
              projectionMatrix_(projectionMatrix),
              color_(color),
              vao_(vao),
              lastTex_(0) {}

    GLuint program_;
    GLint position_;
    GLint uv_;
    GLint posSize_;
    GLint projectionMatrix_;
    GLint color_;
    GLuint vao_;
    GLuint lastTex_;
};

#endif //ANDROIDGLINVESTIGATIONS_SHADER_H