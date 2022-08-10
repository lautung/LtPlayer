//
// Created by lautung on 2022/8/9.
//



#include "LtPlayer.h"


LtPlayer::LtPlayer(const char *data_source, JNICallbakcHelper *helper) {
//    this->data_source = data_source; //data_source在jni会被释放
    this->data_source = new char[strlen(data_source) + 1];
    strcpy(this->data_source, data_source);
    this->helper = helper;

}


LtPlayer::~LtPlayer() {
    if (data_source) {
        delete data_source;
    }
    if (helper) {
        delete helper;
    }
}

//void* (*__start_routine)(void*)
void *prepare_onThread(void *args) {
    auto *player = static_cast<LtPlayer *>(args);
    player->prepare_();
    return nullptr;
}

void LtPlayer::prepare_() {
    // 为什么FFmpeg源码，大量使用上下文Context？
    // 答：因为FFmpeg源码是纯C的，他不像C++、Java ， 上下文的出现是为了贯彻环境，就相当于Java的this能够操作成员
    formatContext = avformat_alloc_context();

    AVDictionary *dictionary = nullptr;
    av_dict_set(&dictionary, "timeout", "5000000", 0); // 单位微妙

    int result = avformat_open_input(&formatContext, this->data_source, nullptr, &dictionary);

    // 释放字典
    av_dict_free(&dictionary);

    if (result) {
        // 把错误信息反馈给Java，回调给Java  Toast【打开媒体格式失败，请检查代码】
        //  JNI 反射回调到Java方法，并提示
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL, av_err2str(result));
        }
        return;
    }

    // 查找媒体中的音视频流的信息, 因为存在 音频流 视频流 字幕流 等。国内一般存在前面两者。
    result = avformat_find_stream_info(formatContext, nullptr);

    if (result < 0) {
        //  JNI 反射回调到Java方法，并提示
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS, av_err2str(result));
        }
        return;
    }

    //根据流信息，流的个数，用循环来找
    for (int stream_index = 0;
         stream_index < formatContext->nb_streams; ++stream_index) { //for start

        //获取媒体流（视频，音频）
        AVStream *stream = formatContext->streams[stream_index];

        /**
        * 从上面的流中 获取 编码解码的【参数】
        * 由于：后面的编码器 解码器 都需要参数（宽高 等等）
        */
        AVCodecParameters *parameters = stream->codecpar;

        //（根据上面的【参数】）获取编解码器
        AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
        if (!codec) {

            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL, av_err2str(result));
            }
        }

        // 编解码器 上下文 （这个才是真正干活的）
        AVCodecContext *codecContext = avcodec_alloc_context3(codec); // AVCodecContext是一张白纸
        if (!codecContext) { // codecContext == NULL   非0即true
            // TODO JNI 反射回调到Java方法，并提示
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL, av_err2str(result));
            }
            return;
        }


        // 他目前是一张白纸（parameters copy codecContext）
        result = avcodec_parameters_to_context(codecContext, parameters);
        if (result < 0) {
            //  JNI 反射回调到Java方法，并提示
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL,
                                av_err2str(result));
            }
            return;
        }

        // 打开解码器
        result = avcodec_open2(codecContext, codec, nullptr);
        if (result) {
            //  JNI 反射回调到Java方法，并提示
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL, av_err2str(result));
            }
            return;
        }

        // 从编解码器参数中，获取流的类型 codec_type === 音频 视频
        if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) { // 音频
            audio_channel = new AudioChannel(stream_index, codecContext);
        } else if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) { // 视频
            video_channel = new VideoChannel(stream_index, codecContext);
            video_channel->setRenderCallback(renderCallback);
        }

    }//for end

    /**
     * 如果流中 没有 音频 也没有 视频 【健壮性校验】
     */
    if (!audio_channel && !video_channel) { //  NULL == audio_channel && NULL == video_channel
        //  JNI 反射回调到Java方法，并提示
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_NOMEDIA, av_err2str(result));
        }
        return;
    }

    // 恭喜你，准备成功，我们的媒体文件 OK了，通知给上层
    if (helper) {
        helper->onPrepared(THREAD_CHILD);
    }

}

void LtPlayer::prepare() {
    // prepare 需要占用大量的资源，不允许占用主线程
    pthread_create(&pid_prepare, 0, prepare_onThread, this);
}

void *start_onThread(void *args) {
    auto *player = static_cast<LtPlayer *>(args);
    player->start_();
    return nullptr;
}


void LtPlayer::start_() {
    while (playing) {
        AVPacket *packet = av_packet_alloc();
        int result = av_read_frame(formatContext, packet);
        if (!result) { //
            if (video_channel && video_channel->stream_index == packet->stream_index) {
                video_channel->packets.insertToQueue(packet);
            } else if (audio_channel && audio_channel->stream_index == packet->stream_index) {
                audio_channel->packets.insertToQueue(packet);
            }
        } else if (result > 0) {
//            TODO if (helper) {
//                helper->onError(THREAD_CHILD, , av_err2str(result))
//            }
        } else if (result == AVERROR_EOF) {

        } else {
            break;
        }
    }
    playing = false;
    if (video_channel) {
        video_channel->stop();
    }
    if (audio_channel) {
        audio_channel->stop();
    }

}


void LtPlayer::start() {
    playing = true;
    if (video_channel) {
        video_channel->start();
    }
    if (audio_channel) {
        audio_channel->start();
    }
    // prepare 需要占用大量的资源，不允许占用主线程
    pthread_create(&pid_prepare, nullptr, start_onThread, this);
}

void LtPlayer::setRenderCallback(RenderCallback callback) {
    this->renderCallback = callback;
}

