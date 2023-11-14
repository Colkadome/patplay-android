#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <memory>

#include "Model.h"
#include "Shader.h"
#include "Time.h"
#include "Sound.h"

struct android_app;

struct PositionAndTime {
    constexpr PositionAndTime(): pos { 0, 0 }, vel { 0, 0 }, time(0) {}
    constexpr PositionAndTime(const float x, const float y, const float vx, const float vy): pos { x, y }, vel { vx, vy }, time(0) {}
    Vector2 pos;
    Vector2 vel;
    float time;
};

class Renderer {
public:
    /*!
     * @param pApp the android_app this Renderer belongs to, needed to configure GL
     */
    inline Renderer(android_app *pApp) :
            app_(pApp),
            display_(EGL_NO_DISPLAY),
            surface_(EGL_NO_SURFACE),
            context_(EGL_NO_CONTEXT),
            width_(0),
            height_(0),
            shaderNeedsNewProjectionMatrix_(true) {
        initRenderer();
    }

    virtual ~Renderer();

    /*!
     * Handles input from the android_app.
     *
     * Note: this will clear the input queue
     */
    void handleInput();

    /*!
     * Runs updates.
     */
    void update();

    /*!
     * Renders all the models in the renderer
     */
    void render();

private:
    /*!
     * Performs necessary OpenGL initialization. Customize this if you want to change your EGL
     * context or application-wide settings.
     */
    void initRenderer();

    /*!
     * @brief we have to check every frame to see if the framebuffer has changed in size. If it has,
     * update the viewport accordingly
     */
    void updateRenderArea();

    /*!
     * Creates the models for this sample. You'd likely load a scene configuration from a file or
     * use some other setup logic in your full game.
     */
    void createModels();

    /*!
     * Pat functions.
     */
    void spawn_pat(float x, float y);
    void spawn_mini_pats(float x, float y);

    Time time_;
    Sound sound_;

    unsigned int pat_count_;

    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;

    std::unique_ptr<Shader> shader_;
    std::vector<Model> models_;

    std::shared_ptr<TextureAsset> regular_pat_texture_;
    std::shared_ptr<TextureAsset> spring_pat_texture_;
    std::shared_ptr<TextureAsset> background_texture_;

    std::vector<PositionAndTime> regular_pats_;
    std::vector<PositionAndTime> red_pats_;
    std::vector<PositionAndTime> mini_pats_;
    std::vector<PositionAndTime> spring_pats_;

    std::vector<std::pair<float, float>> pointer_positions_;

};

#endif //ANDROIDGLINVESTIGATIONS_RENDERER_H