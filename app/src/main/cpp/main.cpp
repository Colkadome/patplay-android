#include <jni.h>

#include "AndroidOut.h"
#include "Renderer.h"

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

extern "C" {

#include <game-activity/native_app_glue/android_native_app_glue.c>

/*!
 * Handles commands sent to this Android application
 * @param pApp the app the commands are coming from
 * @param cmd the command to handle
 */
void handle_cmd(android_app *pApp, int32_t cmd) {
    switch (cmd) {

        case APP_CMD_INIT_WINDOW:
            pApp->userData = new Renderer(pApp);
            break;

        case APP_CMD_TERM_WINDOW:
            if (pApp->userData) {
                auto *pRenderer = reinterpret_cast<Renderer *>(pApp->userData);
                pApp->userData = nullptr;
                delete pRenderer;
            }
            break;

        default:
            break;
    }
}

/*!
 * Enable the motion events you want to handle; not handled events are
 * passed back to OS for further processing. For this example case,
 * only pointer and joystick devices are enabled.
 *
 * @param motionEvent the newly arrived GameActivityMotionEvent.
 * @return true if the event is from a pointer.
 */
bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return sourceClass == AINPUT_SOURCE_CLASS_POINTER; // || sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK;
}

/*!
 * This the main entry point for a native activity
 */
void android_main(struct android_app *pApp) {

    pApp->onAppCmd = handle_cmd;
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);

    // This sets up a typical game/event loop. It will run until the app is destroyed.
    int events;
    android_poll_source *pSource;
    do {

        // Process all pending events.
        if (ALooper_pollAll(0, nullptr, &events, (void **) &pSource) >= 0) {
            if (pSource) {
                pSource->process(pApp, pSource);
            }
        }

        // Check if any user data is associated. This is assigned in handle_cmd.
        if (pApp->userData) {
            auto *pRenderer = reinterpret_cast<Renderer *>(pApp->userData);
            pRenderer->handleInput();
            pRenderer->update();
            pRenderer->render();
        }

    } while (!pApp->destroyRequested);
}

}
