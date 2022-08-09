#ifndef DERRYPLAYER_BASECHANNEL_H
#define DERRYPLAYER_BASECHANNEL_H

extern "C" {
#include <libavcodec/avcodec.h>
};

#include "SafeQueue.h"

class BaseChannel {

public:
    int stream_index; // 音频 或 视频 的下标
    SafeQueue<AVPacket *> packets; // 压缩的 数据包
    SafeQueue<AVFrame *> frames; // 原始的 数据包
    bool isPlaying; // 音频 和 视频 都会有的标记 是否播放
    AVCodecContext *codecContext = 0; // 音频 视频 都需要的 解码器上下文

    BaseChannel(int stream_index, AVCodecContext *codecContext)
            : stream_index(stream_index), codecContext(codecContext);

    virtual ~BaseChannel();
    /**
     * 释放 队列中 所有的 AVPacket *
     * @param packet
     */
    // typedef void (*ReleaseCallback)(T *);
    static void releaseAVPacket(AVPacket **p);

    /**
     * 释放 队列中 所有的 AVFrame *
     * @param packet
     */
    // typedef void (*ReleaseCallback)(T *);
    static void releaseAVFrame(AVFrame **f) ;
};

#endif //DERRYPLAYER_BASECHANNEL_H
