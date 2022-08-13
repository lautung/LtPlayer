//
// Created by lautung on 2022/8/9.
//

#ifndef LTPLAYER_VIDEOCHANNEL_H
#define LTPLAYER_VIDEOCHANNEL_H

#include "BaseChannel.h"
#include "AudioChannel.h"

extern "C" {
#include <libswscale/swscale.h> // 视频画面像素格式转换的模块
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
};

typedef void(*RenderCallback)(uint8_t *, int, int, int); // 函数指针声明定义

class VideoChannel : public BaseChannel {

private:
    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderCallback renderCallback;

    int fps;

    AudioChannel *audioChannel;


public:
    VideoChannel(int stream_index, AVCodecContext *codecContext,AVRational time_base,int fps);

    virtual ~VideoChannel();

    void start();

    void stop();

    void video_decode();

    void video_play();

    void setRenderCallback(RenderCallback callback);

    void setAudioChannel(AudioChannel *audioChannel);
};

#endif