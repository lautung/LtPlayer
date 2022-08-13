//
// Created by lautung on 2022/8/9.
//

#ifndef LTPLAYER_AUDIOCHANNEL_H
#define LTPLAYER_AUDIOCHANNEL_H


#include "BaseChannel.h"
#include "log4c.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "JNICallbakcHelper.h"

extern "C" {
#include <libswresample/swresample.h> // 对音频数据进行转换（重采样）
#include <libavutil/time.h>
};

typedef void (*OnAudioProgress)(int);

class AudioChannel : public BaseChannel {

private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

public:
    double audio_time;

public:
    int out_channels;
    int out_sample_size;
    int out_sample_rate;
    int out_buffers_size;
    uint8_t *out_buffers = nullptr;
    SwrContext *swr_ctx = nullptr;

public:
    // 引擎
    SLObjectItf engineObject = 0;
    // 引擎接口
    SLEngineItf engineInterface = 0;
    // 混音器
    SLObjectItf outputMixObject = 0;
    // 播放器
    SLObjectItf bqPlayerObject = 0;
    // 播放器接口
    SLPlayItf bqPlayerPlay = 0;

    // 播放器队列接口
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = 0;

public:
    AudioChannel(int stream_index, AVCodecContext *codecContext, AVRational time_base);

    virtual ~AudioChannel();

    void start();

    void stop();

    void audio_decode();

    void audio_play();

    int getPCM();
};


#endif //LTPLAYER_AUDIOCHANNEL_H
