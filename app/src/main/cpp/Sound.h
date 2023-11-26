//
// Created by joe on 12/11/2023.
//

#ifndef PAT_PLAY_SOUND_H
#define PAT_PLAY_SOUND_H

#include <future>

#include <android/asset_manager.h>
#include <oboe/Oboe.h>

#include "AudioFile.h"

class Sound: oboe::AudioStreamDataCallback {
public:

    inline Sound() {}

    ~Sound() {
        stop();
    }

    void startAsync(AAssetManager *assetManager);
    void stop();

    void playRegularPat();
    void playRedPat();
    void playExplosion();
    void playSpringPat();
    void playSpringRebound();

private:

    void start();
    bool openStream();
    bool loadSounds(AAssetManager *assetManager);
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override;

    AAssetManager* assetManager_;
    std::shared_ptr<oboe::AudioStream> mAudioStream;
    std::future<void> asyncResult_;

    std::vector<int32_t> regularPatSoundPlays_;
    std::vector<int32_t> redPatSoundPlays_;
    std::vector<int32_t> explosionSoundPlays_;
    std::vector<int32_t> springSoundOnePlays_;
    std::vector<int32_t> springSoundTwoPlays_;
    std::vector<int32_t> springSoundThreePlays_;
    std::vector<int32_t> springReboundSoundOnePlays_;
    std::vector<int32_t> springReboundSoundTwoPlays_;
    std::vector<int32_t> springReboundSoundThreePlays_;

    std::unique_ptr<AudioFile<float>> regularPatSound_;
    std::unique_ptr<AudioFile<float>> redPatSound_;
    std::unique_ptr<AudioFile<float>> explosionSound_;
    std::unique_ptr<AudioFile<float>> springSoundOne_;
    std::unique_ptr<AudioFile<float>> springSoundTwo_;
    std::unique_ptr<AudioFile<float>> springSoundThree_;
    std::unique_ptr<AudioFile<float>> springReboundSoundOne_;
    std::unique_ptr<AudioFile<float>> springReboundSoundTwo_;
    std::unique_ptr<AudioFile<float>> springReboundSoundThree_;

};

#endif //PAT_PLAY_SOUND_H
