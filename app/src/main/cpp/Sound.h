//
// Created by joe on 12/11/2023.
//

#ifndef PAT_PLAY_SOUND_H
#define PAT_PLAY_SOUND_H

#include <oboe/Oboe.h>

class Sound {
public:

    inline Sound() {}

    ~Sound() {
        stop();
    }

    void start();
    void stop();

    void playRegularPat();
    void playRedPat();
    void playExplosion();
    void playSpringPat();
    void playSpringRebound();

private:

    bool openStream();
    bool closeStream();

    std::shared_ptr<oboe::AudioStream> mAudioStream;

};

#endif //PAT_PLAY_SOUND_H
