#ifndef DERRYPLAYER_BASECHANNEL_H
#define DERRYPLAYER_BASECHANNEL_H

extern "C" {
#include <libavcodec/avcodec.h>
};

#include "SafeQueue.h"
#include "JNICallbakcHelper.h"

class BaseChannel {
public:
    AVRational time_base;

public:
    int stream_index; // 音频 或 视频 的下标
    SafeQueue<AVPacket *> packets; // 压缩的 数据包
    SafeQueue<AVFrame *> frames; // 原始的 数据包
    bool isPlaying = false; // 音频 和 视频 都会有的标记 是否播放
    AVCodecContext *codecContext = 0; // 音频 视频 都需要的 解码器上下文

    JNICallbakcHelper *jniCallbakcHelper = nullptr;

    void setJNICallbakcHelper(JNICallbakcHelper *callbakcHelper) {
        this->jniCallbakcHelper = callbakcHelper;
    }

    BaseChannel(int stream_index, AVCodecContext *codecContext, AVRational time_base)
            : stream_index(
            stream_index), codecContext(codecContext), time_base(time_base) {
        packets.setReleaseCallback(releaseAVPacket); // 给队列设置Callback，Callback释放队列里面的数据
        frames.setReleaseCallback(releaseAVFrame); // 给队列设置Callback，Callback释放队列里面的数据
    }

    ~BaseChannel() {
        packets.clear();
        frames.clear();
    };
    /**
     * 释放 队列中 所有的 AVPacket *
     * @param packet
     */
    // typedef void (*ReleaseCallback)(T *);
    static void releaseAVPacket(AVPacket **p) {
        if (p) {
            av_packet_free(p); // 释放队列里面的 T == AVPacket
            *p = nullptr;
        }
    };

    /**
     * 释放 队列中 所有的 AVFrame *
     * @param packet
     */
    // typedef void (*ReleaseCallback)(T *);
    static void releaseAVFrame(AVFrame **f) {
        if (f) {
            av_frame_free(f); // 释放队列里面的 T == AVFrame
            *f = nullptr;
        }
    };
};


#endif //DERRYPLAYER_BASECHANNEL_H
