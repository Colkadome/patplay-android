//
// Created by joe on 12/11/2023.
//

#include "Sound.h"

#include "AndroidOut.h"

void Sound::start() {

    openStream();

}

void Sound::stop() {

    closeStream();

}

bool Sound::openStream() {

    oboe::AudioStreamBuilder builder;
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setFormatConversionAllowed(true);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setSampleRate(48000);
    builder.setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium);
    builder.setChannelCount(2);

    oboe::Result result = builder.openStream(mAudioStream);
    if (result != oboe::Result::OK) {
        aout << "Failed to open stream. Error: " << convertToText(result) << std::endl;
        return false;
    }

    return true;
}

bool Sound::closeStream() {

    return true;
}

void Sound::playRegularPat() {

}

void Sound::playRedPat() {

}

void Sound::playExplosion() {

}

void Sound::playSpringPat() {

}

void Sound::playSpringRebound() {

}
