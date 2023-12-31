#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <android/imagedecoder.h>

#include "AndroidOut.h"
#include "Shader.h"
#include "Utility.h"
#include "TextureAsset.h"
#include "Save.h"

//! executes glGetString and outputs the result to logcat
#define PRINT_GL_STRING(s) {aout << #s": "<< glGetString(s) << std::endl;}

/*!
 * @brief if glGetString returns a space separated list of elements, prints each one on a new line
 *
 * This works by creating an istringstream of the input c-style string. Then that is used to create
 * a vector -- each element of the vector is a new element in the input string. Finally a foreach
 * loop consumes this and outputs it to logcat using @a aout
 */
#define PRINT_GL_STRING_AS_LIST(s) { \
std::istringstream extensionStream((const char *) glGetString(s));\
std::vector<std::string> extensionList(\
        std::istream_iterator<std::string>{extensionStream},\
        std::istream_iterator<std::string>());\
aout << #s":\n";\
for (auto& extension: extensionList) {\
    aout << extension << "\n";\
}\
aout << std::endl;\
}

/*!
 * Gets a random velocity.
 */
const int RED_PAT = 2;
const int SPRING_PAT = 1;
const int REGULAR_PAT = 0;
int rand_pat() {
    int r = rand() % 400;
    if (r < 2) {
        return RED_PAT;
    } else if (r == 2) {
        return SPRING_PAT;
    } else {
        return REGULAR_PAT;
    }
}
inline float rand_vel() {
    return ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 2.0) - 1.0;
}

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
}

void Renderer::render() {
    // Check to see if the surface has changed size. This is _necessary_ to do every frame when
    // using immersive mode as you'll get no other notification that your renderable area has
    // changed.
    updateRenderArea();

    // When the renderable area changes, the projection matrix has to also be updated. This is true
    // even if you change from the sample orthographic projection matrix as your aspect ratio has
    // likely changed.
    auto w = (float) width_;
    auto h = (float) height_;
    if (shaderNeedsNewProjectionMatrix_) {
        float projectionMatrix[16] = {0};
        Utility::buildOrthographicMatrix(projectionMatrix, w, h);
        shader_->setProjectionMatrix(projectionMatrix);
        shaderNeedsNewProjectionMatrix_ = false;
    }

    // clear the color buffer
    //glClear(GL_COLOR_BUFFER_BIT);

    // Draw the background.
    bool setWhite = true;
    auto max_dim = fmax(w, h);
    shader_->setColor(1, 1, 1, 1);
    shader_->setTexture(background_texture_->getTextureID());
    shader_->drawShape(w / 2, h / 2, max_dim, max_dim);

    // Render the regular pats.
    float scale = 48;
    if (!regular_pats_.empty() || !red_pats_.empty() || !mini_pats_.empty()) {
        shader_->setTexture(regular_pat_texture_->getTextureID());
    }
    for (auto posTime : regular_pats_) {
        shader_->drawShape(posTime.pos.x, posTime.pos.y, scale * 2, scale * 2);
    }
    if (!red_pats_.empty() || !mini_pats_.empty()) {
        shader_->setColor(1, 0, 0, 1);
        setWhite = false;
    }
    for (auto posTime : mini_pats_) {
        shader_->drawShape(posTime.pos.x, posTime.pos.y, scale, scale);
    }
    for (auto posTime : red_pats_) {
        shader_->drawShape(posTime.pos.x, posTime.pos.y, scale * 4, scale * 4);
    }

    // Render the spring pats.
    if (!spring_pats_.empty()) {
        shader_->setColor(1, 1, 0.5, 1);
        setWhite = false;
        shader_->setTexture(spring_pat_texture_->getTextureID());
        for (auto posTime : spring_pats_) {
            shader_->drawShape(posTime.pos.x, posTime.pos.y, scale * 3, scale * 3);
        }
    }

    // Render the pat count.
    // We want to minimize texture swapping, hence the weird for-loops.
    std::string countStr = std::to_string(save_.getPatCount());
    for (char n = '0'; n <= '9'; n++) {
        bool changedTexture = false;
        for (int i = 0; i < countStr.size(); i++) {
            if (n == countStr[i]) {
                if (!changedTexture) {
                    changedTexture = true;
                    int tex = 0;
                    switch (n) {
                        case '0': tex = zero_texture_->getTextureID(); break;
                        case '1': tex = one_texture_->getTextureID(); break;
                        case '2': tex = two_texture_->getTextureID(); break;
                        case '3': tex = three_texture_->getTextureID(); break;
                        case '4': tex = four_texture_->getTextureID(); break;
                        case '5': tex = five_texture_->getTextureID(); break;
                        case '6': tex = six_texture_->getTextureID(); break;
                        case '7': tex = seven_texture_->getTextureID(); break;
                        case '8': tex = eight_texture_->getTextureID(); break;
                        case '9': tex = nine_texture_->getTextureID(); break;
                    }
                    shader_->setTexture(tex);
                }
                if (!setWhite) {
                    setWhite = true;
                    shader_->setColor(1, 1, 1, 1);
                }
                shader_->drawShape(32 + (i * 32), height_ - 32, 32, 32);
            }
        }
    }

    // Present the rendered image. This is an implicit glFlush.
    auto swapResult = eglSwapBuffers(display_, surface_);
    assert(swapResult == EGL_TRUE);
}

void Renderer::postRender() {

    if (needsSave_) {
        save_.savePatCount(app_->activity->internalDataPath);
        needsSave_ = false;
    }

}

void Renderer::update() {

    auto w = (float) width_;
    auto h = (float) height_;
    float dt = time_.get_dt();
    float gravity = 2.0;
    float baseSpeed = 1024.0;
    float redSpeed = 128.0;
    float springStrength = 2.0;
    float maxVelocity = 64.0;

    // Process regular pats.
    auto last = regular_pats_.size();
    for (auto i = 0; i < last; i++) {
        while (last > i && regular_pats_[i].time > 2.0) {
            last -= 1;
            regular_pats_[i] = regular_pats_[last];
        }
        regular_pats_[i].time += dt;
        regular_pats_[i].vel.y -= gravity * dt;
        regular_pats_[i].pos.x += regular_pats_[i].vel.x * baseSpeed * dt;
        regular_pats_[i].pos.y += regular_pats_[i].vel.y * baseSpeed * dt;
    }
    regular_pats_.resize(last);

    // Process mini pats.
    last = mini_pats_.size();
    for (auto i = 0; i < last; i++) {
        while (last > i && mini_pats_[i].time > 1.0) {
            last -= 1;
            mini_pats_[i] = mini_pats_[last];
        }
        mini_pats_[i].time += dt;
        mini_pats_[i].vel.y -= gravity * dt;
        mini_pats_[i].pos.x += mini_pats_[i].vel.x * baseSpeed * dt;
        mini_pats_[i].pos.y += mini_pats_[i].vel.y * baseSpeed * dt;
    }
    mini_pats_.resize(last);

    // Process red pats.
    last = red_pats_.size();
    for (auto i = 0; i < last; i++) {
        while (last > i && red_pats_[i].time > 1.0) {
            last -= 1;
            spawn_mini_pats(red_pats_[i].pos.x, red_pats_[i].pos.y);
            red_pats_[i] = red_pats_[last];
        }
        red_pats_[i].time += dt;
        red_pats_[i].vel.y -= gravity * dt;
        red_pats_[i].pos.x += red_pats_[i].vel.x * redSpeed * dt;
        red_pats_[i].pos.y += red_pats_[i].vel.y * redSpeed * dt;
    }
    red_pats_.resize(last);

    // Process spring pats.
    last = spring_pats_.size();
    for (auto i = 0; i < last; i++) {
        while (last > i && spring_pats_[i].time > 4.0) {
            last -= 1;
            spring_pats_[i] = spring_pats_[last];
        }
        spring_pats_[i].time += dt;
        spring_pats_[i].vel.y -= gravity * dt;
        spring_pats_[i].pos.x += spring_pats_[i].vel.x * baseSpeed * dt;
        spring_pats_[i].pos.y += spring_pats_[i].vel.y * baseSpeed * dt;

        // Reverse velocities if the edge is reached.
        bool hit_edge = false;
        if (spring_pats_[i].pos.x < 0) {
            spring_pats_[i].vel.x = fmin(spring_pats_[i].vel.x * -springStrength, maxVelocity);
            spring_pats_[i].pos.x = 0;
            hit_edge = true;
        } else if (spring_pats_[i].pos.x > w) {
            spring_pats_[i].vel.x = fmax(spring_pats_[i].vel.x * -springStrength, -maxVelocity);
            spring_pats_[i].pos.x = w;
            hit_edge = true;
        }
        if (spring_pats_[i].pos.y < 0) {
            spring_pats_[i].vel.y = fmin(spring_pats_[i].vel.y * -springStrength, maxVelocity);
            spring_pats_[i].pos.y = 0;
            hit_edge = true;
        } else if (spring_pats_[i].pos.y > h) {
            spring_pats_[i].vel.y = fmax(spring_pats_[i].vel.y * -springStrength, -maxVelocity);
            spring_pats_[i].pos.y = h;
            hit_edge = true;
        }
        if (hit_edge) {
            sound_.playSpringRebound();
        }
    }
    spring_pats_.resize(last);

    // Decrement save timer.
    if (timeUntilSave_ > 0.0) {
        timeUntilSave_ -= dt;
        if (timeUntilSave_ <= 0.0) {
            needsSave_ = true;
        }
    }

}

void Renderer::increment_counter(int c) {

    save_.incrementPatCount(c);

    if (timeUntilSave_ <= 0.0) {
        timeUntilSave_ = 1.0;
    }

}

void Renderer::spawn_pat(float x, float y) {
    int pat = rand_pat();
    if (pat == RED_PAT) {
        red_pats_.emplace_back(x,  y, rand_vel(), rand_vel());
        increment_counter(1);
        sound_.playRedPat();
    } else if (pat == SPRING_PAT) {
        spring_pats_.emplace_back(x,  y, rand_vel(), rand_vel());
        spring_pats_.emplace_back(x,  y, rand_vel(), rand_vel());
        spring_pats_.emplace_back(x,  y, rand_vel(), rand_vel());
        increment_counter(3);
        sound_.playSpringPat();
    } else {
        regular_pats_.emplace_back(x,  y, rand_vel(), rand_vel());
        increment_counter(1);
        sound_.playRegularPat();
    }
}

void Renderer::spawn_mini_pats(float x, float y) {
    float speed = 4.0;
    int count = 10;
    for (auto i = 0; i < count; i++) {
        mini_pats_.emplace_back(x, y, rand_vel() * speed, rand_vel() * speed);
    }
    increment_counter(count);
    sound_.playExplosion();
}

void Renderer::initRenderer() {

    // Choose your render attributes.
    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    // The default display is probably what you want on Android
    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    // figure out how many configs there are
    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

    // get the list of configurations
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    // Find a config we like.
    // Could likely just grab the first if we don't care about anything else in the config.
    // Otherwise hook in your own heuristic
    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {

                    aout << "Found config with " << red << ", " << green << ", " << blue << ", "
                         << depth << std::endl;
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    aout << "Found " << numConfigs << " configs" << std::endl;
    aout << "Chose " << config << std::endl;

    // create the proper window surface
    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);

    // Create a GLES 3 context
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);

    // get some window metrics
    auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
    assert(madeCurrent);

    display_ = display;
    surface_ = surface;
    context_ = context;

    // make width and height invalid so it gets updated the first frame in @a updateRenderArea()
    width_ = -1;
    height_ = -1;

    PRINT_GL_STRING(GL_VENDOR);
    PRINT_GL_STRING(GL_RENDERER);
    PRINT_GL_STRING(GL_VERSION);
    PRINT_GL_STRING_AS_LIST(GL_EXTENSIONS);

    shader_ = std::unique_ptr<Shader>(Shader::loadShader());
    assert(shader_);

    // Note: there's only one shader in this demo, so I'll activate it here. For a more complex game
    // you'll want to track the active shader and activate/deactivate it as necessary
    shader_->activate();

    glClearColor(0, 0, 0, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Load textures.
    auto assetManager = app_->activity->assetManager;
    regular_pat_texture_ = TextureAsset::loadAsset(assetManager, "jpg/pat.jpeg", 1);
    spring_pat_texture_ = TextureAsset::loadAsset(assetManager, "jpg/springpat.jpeg", 1);
    background_texture_ = TextureAsset::loadAsset(assetManager, "jpg/background.jpeg", 0);
    zero_texture_ = TextureAsset::loadAsset(assetManager, "png/zero.png", 2);
    one_texture_ = TextureAsset::loadAsset(assetManager, "png/one.png", 2);
    two_texture_ = TextureAsset::loadAsset(assetManager, "png/two.png", 2);
    three_texture_ = TextureAsset::loadAsset(assetManager, "png/three.png", 2);
    four_texture_ = TextureAsset::loadAsset(assetManager, "png/four.png", 2);
    five_texture_ = TextureAsset::loadAsset(assetManager, "png/five.png", 2);
    six_texture_ = TextureAsset::loadAsset(assetManager, "png/six.png", 2);
    seven_texture_ = TextureAsset::loadAsset(assetManager, "png/seven.png", 2);
    eight_texture_ = TextureAsset::loadAsset(assetManager, "png/eight.png", 2);
    nine_texture_ = TextureAsset::loadAsset(assetManager, "png/nine.png", 2);

    // Init timer by jigging it.
    time_.get_dt();

    // Init sound.
    sound_.startAsync(assetManager);

    // Init save.
    save_.init(app_->activity->internalDataPath);

}

void Renderer::updateRenderArea() {

    EGLint width, height;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);
        shaderNeedsNewProjectionMatrix_ = true;
    }

}

/**
 * @brief Create any demo models we want for this demo.
 */
void Renderer::createModels() {

    // empty.

}

void Renderer::handleInput() {

    // handle all queued inputs
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        return;  // no inputs yet.
    }

    // handle motion events (motionEventsCounts can be 0).
    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;

        // determine the action type and process the event accordingly.
        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN: {
                // get the x and y position of this event if it is not ACTION_MOVE. (?)
                auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                auto &pointer = motionEvent.pointers[pointerIndex];
                auto x = GameActivityPointerAxes_getX(&pointer);
                auto y = GameActivityPointerAxes_getY(&pointer);
                pointer_positions_.resize(pointerIndex + 1);
                pointer_positions_[pointerIndex] = { x, y };
                spawn_pat(x, (float) height_ - y);
                break;
            }

            case AMOTION_EVENT_ACTION_CANCEL:
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP: {
                // No erasing done. We just assume someone can't point more than 10 times.
                break;
            }

            case AMOTION_EVENT_ACTION_MOVE: {
                // There is no pointer index for ACTION_MOVE, only a snapshot of
                // all active pointers; app needs to cache previous active pointers
                // to figure out which ones are actually moved.
                for (auto index = 0; index < motionEvent.pointerCount && index < pointer_positions_.size(); index++) {
                    auto pointer = motionEvent.pointers[index];
                    auto x = GameActivityPointerAxes_getX(&pointer);
                    auto y = GameActivityPointerAxes_getY(&pointer);
                    auto x_old = pointer_positions_[index].first;
                    auto y_old = pointer_positions_[index].second;
                    if (x != x_old || y != y_old) {
                        pointer_positions_[index] = { x, y };
                        spawn_pat(x, (float) height_ - y);
                    }
                }
                break;
            }
        }
    }
    // clear the motion input count in this buffer for main thread to re-use.
    android_app_clear_motion_events(inputBuffer);

    // handle input key events.
//    for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
//        auto &keyEvent = inputBuffer->keyEvents[i];
//        aout << "Key: " << keyEvent.keyCode <<" ";
//        switch (keyEvent.action) {
//            case AKEY_EVENT_ACTION_DOWN:
//                aout << "Key Down";
//                break;
//            case AKEY_EVENT_ACTION_UP:
//                aout << "Key Up";
//                break;
//            case AKEY_EVENT_ACTION_MULTIPLE:
//                // Deprecated since Android API level 29.
//                aout << "Multiple Key Actions";
//                break;
//            default:
//                aout << "Unknown KeyEvent Action: " << keyEvent.action;
//        }
//        aout << std::endl;
//    }

    // clear the key input count too.
    android_app_clear_key_events(inputBuffer);

}
