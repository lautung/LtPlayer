//
// Created by lautung on 2022/8/9.
//

#include "VideoChannel.h"


VideoChannel::VideoChannel(int stream_index, AVCodecContext *codecContext) : BaseChannel(
        stream_index, codecContext) {

}

VideoChannel::~VideoChannel() {

}

void *task_video_decode(void *args) {
    auto *video_channel = static_cast<VideoChannel *>(args);
    video_channel->video_decode();
    return nullptr;
}

void VideoChannel::video_decode() {
    AVPacket *pkt = nullptr;
    while (isPlaying) {
        int r = packets.getQueueAndDel(pkt); // 阻塞式函数
        if (!isPlaying) {
            break;  // 如果关闭了播放，跳出循环，releaseAVPacket(&pkt);
        }

        if (!r) { // r == 0
            continue;  // 哪怕是没有成功，也要继续（假设：你生产太慢(压缩包加入队列)，我消费就等一下你）
        }

        r = avcodec_send_packet(codecContext, pkt); // 第一步：把我们的 压缩包 AVPack发送给 FFmpeg缓存区

        // FFmpeg源码内部 缓存了一份pkt副本，所以我才敢大胆的释放
        releaseAVPacket(&pkt);

        if (r) { // r != 0
            break; // avcodec_send_packet出现了错误
        }

        // 第二步：读取 FFmpeg缓存区 A里面的 原始包 ，有可能读不到，为什么？ 内部缓冲区 会 运作过程比较慢
        AVFrame *frame = av_frame_alloc();
        r = avcodec_receive_frame(codecContext, frame);
        if (r == AVERROR(EAGAIN)) {
            continue; // B帧 B帧参考前面成功  B帧参考后面失败   可能是P帧还没有出来， 你等等  你再拿一次 可能就拿到了
        } else if (r != 0) {
            break; // 出错误了
        }
        // 终于拿到 原始包了，加入队列
        frames.insertToQueue(frame);
    } // while end

    releaseAVPacket(&pkt);
}

void *task_video_play(void *args) {
    auto *video_channel = static_cast<VideoChannel *>(args);
    video_channel->video_play();
    return nullptr;
}


void VideoChannel::video_play() {
    AVFrame * frame = nullptr;
    uint8_t *dst_date[4]; // RGBA
    int dst_linesize[4]; // RGBA

    while (isPlaying) {
        int ret = frames.getQueueAndDel(frame);
        if (!isPlaying) {
            break;  // 如果关闭了播放，跳出循环，releaseAVPacket(&pkt);
        }

        if (!ret) { // r == 0
            continue;  // 哪怕是没有成功，也要继续（假设：你生产太慢(原始包加入队列)，我消费就等一下你）
        }

        // 格式转换 yuv ----> rgba
        sws_scale(
                nullptr,
                // 下面是 输入环节  YUV的数据
                frame->data, frame->linesize,
                0, codecContext->height,

                // 下面是 输出环节 成果：RGBA数据 Android SurfaceView播放画面
                dst_date,
                dst_linesize
        );
    }
}


void VideoChannel::start() {
    isPlaying = true;
    packets.setWork(1);
    frames.setWork(1);
    pthread_create(&pid_video_decode, nullptr, task_video_decode, this);
    pthread_create(&pid_video_play, nullptr, task_video_play, this);
}

void VideoChannel::stop() {

}


