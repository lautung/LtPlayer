//
// Created by lautung on 2022/8/9.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int stream_index, AVCodecContext *codecContext) : BaseChannel(
        stream_index, codecContext) {

}


void AudioChannel::start() {

}

void AudioChannel::stop() {

}


AudioChannel::~AudioChannel() = default;


