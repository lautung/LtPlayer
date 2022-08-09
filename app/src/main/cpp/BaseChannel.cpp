//
// Created by lautung on 2022/8/9.
//

#include "BaseChannel.h"

BaseChannel::BaseChannel(int stream_index, AVCodecContext *codecContext) {
    packets.setReleaseCallback(releaseAVPacket); // 给队列设置Callback，Callback释放队列里面的数据
    frames.setReleaseCallback(releaseAVFrame); // 给队列设置Callback，Callback释放队列里面的数据
}

void BaseChannel::releaseAVFrame(AVFrame **f) {
    if (f) {
        av_frame_free(f); // 释放队列里面的 T == AVFrame
        *f = 0;
    }
}

void BaseChannel::releaseAVPacket(AVPacket **p) {
    if (p) {
        av_packet_free(p); // 释放队列里面的 T == AVPacket
        *p = 0;
    }
}

BaseChannel::~BaseChannel() {
    packets.clear();
    frames.clear();
}

