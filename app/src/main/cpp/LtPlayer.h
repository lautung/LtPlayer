//
// Created by lautung on 2022/8/9.
//

#ifndef LTPLAYER_LTPLAYER_H
#define LTPLAYER_LTPLAYER_H

#include <string.h>
#include <pthread.h>
#include "AudioChannel.h"
#include "VideoChannel.h"
#include "JNICallbakcHelper.h"

extern "C" {
#include <libavformat/avformat.h>
};

class LtPlayer {
private:
    char *data_source = nullptr;
    pthread_t pid{};
    AVFormatContext *formatContext = nullptr;
    AudioChannel *audio_channel = nullptr;
    VideoChannel *video_channel = nullptr;
    JNICallbakcHelper *helper;

public:
    LtPlayer(const char *data_source, JNICallbakcHelper *helper);

    ~LtPlayer();

    void prepare();


    void prepare_();
    void start();


};


#endif //LTPLAYER_LTPLAYER_H
