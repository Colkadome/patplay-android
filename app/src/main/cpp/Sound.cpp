//
// Created by joe on 12/11/2023.
//

#include "Sound.h"

#include <future>

#include <android/asset_manager.h>

#include "AudioFile.h"
#include "AndroidOut.h"

std::unique_ptr<AudioFile<float>> loadSoundFile(AAssetManager *assetManager, const std::string &assetPath) {

    // Try and open asset.
    AAsset *asset = AAssetManager_open(assetManager, assetPath.c_str(), AASSET_MODE_UNKNOWN);
    if (!asset) {
        aout << "Failed to open asset " << assetPath << std::endl;
        return nullptr;
    }

    // Get file data.
    auto data = AAsset_getBuffer(asset);
    auto size = AAsset_getLength(asset);

    // Copy into audio file.
    std::vector<uint8_t> data_in_vec((uint8_t*)data, (uint8_t*)data + size);
    auto audioFile = std::make_unique<AudioFile<float>>();
    audioFile->loadFromMemory(data_in_vec);
    AAsset_close(asset);

    return audioFile;

}

void Sound::startAsync(AAssetManager *assetManager) {
    assetManager_ = assetManager;
    asyncResult_ = std::async(&Sound::start, this);
}

void Sound::start() {
    if (openStream()) {
        loadSounds(assetManager_);
        oboe::Result result = mAudioStream->requestStart();
        if (result != oboe::Result::OK) {
            aout << "Failed to start audio" << std::endl;
        }
    }
    assetManager_ = nullptr;
}

void Sound::stop() {

    if (mAudioStream) {
        mAudioStream->stop();
        mAudioStream->close();
    }

    mAudioStream = nullptr;

}

bool Sound::openStream() {

    // Build the stream.

    oboe::AudioStreamBuilder builder;
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setFormatConversionAllowed(true);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setSampleRate(44100);
    builder.setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium);
    builder.setChannelCount(2);
    builder.setDataCallback(this);

    oboe::Result result = builder.openStream(mAudioStream);
    if (result != oboe::Result::OK) {
        aout << "Failed to open stream. Error: " << convertToText(result) << std::endl;
        return false;
    }

    mAudioStream->setDelayBeforeCloseMillis(500);

    return true;
}

bool Sound::loadSounds(AAssetManager *assetManager) {

    regularPatSound_ = loadSoundFile(assetManager, "wav/pat.wav");
    redPatSound_ = loadSoundFile(assetManager, "wav/redpat.wav");
    explosionSound_ = loadSoundFile(assetManager, "wav/explode.wav");
    springSoundOne_ = loadSoundFile(assetManager, "wav/springypat1.wav");
    springSoundTwo_ = loadSoundFile(assetManager, "wav/springypat2.wav");
    springSoundThree_ = loadSoundFile(assetManager, "wav/springypat3.wav");
    springReboundSoundOne_ = loadSoundFile(assetManager, "wav/spring1.wav");
    springReboundSoundTwo_ = loadSoundFile(assetManager, "wav/spring2.wav");
    springReboundSoundThree_ = loadSoundFile(assetManager, "wav/spring3.wav");

    return true;

}

void playSoundArray(float* outputBuffer, AudioFile<float>* audioFile, std::vector<int32_t> &cursors, int numChannels, int numFrames) {

    auto channelCount = audioFile->getNumChannels();
    if (channelCount > numChannels) {
        return;
    }

    // Apply the audio data, starting from the cursor positions,
    // and finishing either at the end of the file, or the end of "numFrames".
    auto frameCount = audioFile->getNumSamplesPerChannel();
    auto last = cursors.size();
    for (auto i = 0; i < last; i++) {
        int32_t p = cursors[i];
        for (auto f = 0; p < frameCount && f < numFrames; p++, f++) {
            for (auto c = 0; c < channelCount; c++) {
                outputBuffer[(f * numChannels) + c] += audioFile->samples[c][p];
            }
        }
        cursors[i] = p;
    }

    // Filter vector if the cursors have reached the end of the file.
    for (auto i = 0; i < last; i++) {
        while (last > i && cursors[i] >= frameCount) {
            last -= 1;
            cursors[i] = cursors[last];
        }
    }
    cursors.resize(last);

}

oboe::DataCallbackResult Sound::onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) {

    if (!oboeStream) {
        return oboe::DataCallbackResult::Stop;
    }

    switch (oboeStream->getState()) {
        case oboe::StreamState::Closed:
        case oboe::StreamState::Closing:
        case oboe::StreamState::Disconnected:
        case oboe::StreamState::Stopped:
        case oboe::StreamState::Stopping:
            return oboe::DataCallbackResult::Stop;
    }

    int numChannels = oboeStream->getChannelCount();
    int numSamples = numFrames * numChannels;

    // Clear the sound.
    auto *outputBuffer = static_cast<float*>(audioData);
    memset(outputBuffer, 0, sizeof(float) * numSamples);

    // For each sound, add the sound data to the stream.
    // Pretty dumb method but there aren't many sounds.
    if (regularPatSound_ && !regularPatSoundPlays_.empty()) {
        playSoundArray(outputBuffer, regularPatSound_.get(), regularPatSoundPlays_, numChannels, numFrames);
    }
    if (redPatSound_ && !redPatSoundPlays_.empty()) {
        playSoundArray(outputBuffer, redPatSound_.get(), redPatSoundPlays_, numChannels, numFrames);
    }
    if (explosionSound_ && !explosionSoundPlays_.empty()) {
        playSoundArray(outputBuffer, explosionSound_.get(), explosionSoundPlays_, numChannels, numFrames);
    }
    if (springSoundOne_ && !springSoundOnePlays_.empty()) {
        playSoundArray(outputBuffer, springSoundOne_.get(), springSoundOnePlays_, numChannels, numFrames);
    }
    if (springSoundTwo_ && !springSoundTwoPlays_.empty()) {
        playSoundArray(outputBuffer, springSoundTwo_.get(), springSoundTwoPlays_, numChannels, numFrames);
    }
    if (springSoundThree_ && !springSoundThreePlays_.empty()) {
        playSoundArray(outputBuffer, springSoundThree_.get(), springSoundThreePlays_, numChannels, numFrames);
    }
    if (springReboundSoundOne_ && !springReboundSoundOnePlays_.empty()) {
        playSoundArray(outputBuffer, springReboundSoundOne_.get(), springReboundSoundOnePlays_, numChannels, numFrames);
    }
    if (springReboundSoundTwo_ && !springReboundSoundTwoPlays_.empty()) {
        playSoundArray(outputBuffer, springReboundSoundTwo_.get(), springReboundSoundTwoPlays_, numChannels, numFrames);
    }
    if (springReboundSoundThree_ && !springReboundSoundThreePlays_.empty()) {
        playSoundArray(outputBuffer, springReboundSoundThree_.get(), springReboundSoundThreePlays_, numChannels, numFrames);
    }

    return oboe::DataCallbackResult::Continue;
}

void Sound::playRegularPat() {
    if (regularPatSound_) {
        regularPatSoundPlays_.push_back(0);
    }
}

void Sound::playRedPat() {
    if (redPatSound_) {
        redPatSoundPlays_.push_back(0);
    }
}

void Sound::playExplosion() {
    if (redPatSound_) {
        explosionSoundPlays_.push_back(0);
    }
}

void Sound::playSpringPat() {
    int n = rand() % 3;
    if (n == 2) {
        springSoundOnePlays_.push_back(0);
    } else if (n == 1) {
        springSoundTwoPlays_.push_back(0);
    } else {
        springSoundThreePlays_.push_back(0);
    }
}

void Sound::playSpringRebound() {
    int n = rand() % 3;
    if (n == 2) {
        springReboundSoundOnePlays_.push_back(0);
    } else if (n == 1) {
        springReboundSoundTwoPlays_.push_back(0);
    } else {
        springReboundSoundThreePlays_.push_back(0);
    }
}
