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
        avformat_close_input(&formatContext);
        return;
    }

    // 查找媒体中的音视频流的信息, 因为存在 音频流 视频流 字幕流 等。国内一般存在前面两者。
    result = avformat_find_stream_info(formatContext, nullptr);

    if (result < 0) {
        //  JNI 反射回调到Java方法，并提示
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS, av_err2str(result));
        }
        avformat_close_input(&formatContext);
        return;
    }

    this->duration = formatContext->duration / AV_TIME_BASE; //ffmepg的时间必须要使用 avrational来处理。

    AVCodecContext *codecContext = nullptr;
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
            avformat_close_input(&formatContext);
        }

        // 编解码器 上下文 （这个才是真正干活的）
        codecContext = avcodec_alloc_context3(codec); // AVCodecContext是一张白纸
        if (!codecContext) { // codecContext == NULL   非0即true
            // TODO JNI 反射回调到Java方法，并提示
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL, av_err2str(result));
            }
            avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
            avformat_close_input(&formatContext);
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
            avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
            avformat_close_input(&formatContext);
            return;
        }

        // 打开解码器
        result = avcodec_open2(codecContext, codec, nullptr);
        if (result) {
            //  JNI 反射回调到Java方法，并提示
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL, av_err2str(result));
            }
            avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
            avformat_close_input(&formatContext);
            return;
        }

        AVRational time_base = stream->time_base;

        // 从编解码器参数中，获取流的类型 codec_type === 音频 视频
        if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) { // 音频
            audio_channel = new AudioChannel(stream_index, codecContext, time_base);
            if (this->duration != 0) { // 非直播，才有意义把 JNICallbackHelper传递过去
                audio_channel->setJNICallbakcHelper(helper);
            }
        } else if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) { // 视频

            //如果是封面流，也是不参与视频的解码。
            if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                continue;
            }
            AVRational fps_rational = stream->avg_frame_rate;
            int fps = av_q2d(fps_rational);
            video_channel = new VideoChannel(stream_index, codecContext, time_base, fps);
            video_channel->setRenderCallback(renderCallback);
            if (this->duration != 0) { // 非直播，才有意义把 JNICallbackHelper传递过去
                video_channel->setJNICallbakcHelper(helper);
            }
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
        if (codecContext) {
            avcodec_free_context(&codecContext); // 释放此上下文 avcodec 他会考虑到，你不用管*codec
        }
        avformat_close_input(&formatContext);
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
        /*
         * 判断视频packet队列是否大于100，防止 生产者 太快 ，消费者 太慢 导致的内存占用问题。
         */
        if (video_channel && video_channel->packets.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }

        if (audio_channel && audio_channel->packets.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }

        //创建一个 audio或者video的packet. av_packet_alloc为它分配内存空间。
        AVPacket *packet = av_packet_alloc();

        //av_read_frame从 mp4或其它封装格式中读取AVPacket给变量packet。result判断是否成功
        int result = av_read_frame(formatContext, packet);
        if (!result) { //
            if (video_channel && video_channel->stream_index == packet->stream_index) {
                video_channel->packets.insertToQueue(packet);
            } else if (audio_channel && audio_channel->stream_index == packet->stream_index) {
                audio_channel->packets.insertToQueue(packet);
            }
        } else if (result > 0) { // 错误
//            TODO if (helper) {
//                helper->onError(THREAD_CHILD, , av_err2str(result))
//            }
        } else if (result == AVERROR_EOF) {
            /*
             * AVERROR_EOF读完了，不代表播放完成
             */
            if (video_channel->packets.empty() && audio_channel->packets.empty()) {
                break;
            }

        } else {
            break;
        }


    } // while end
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

    if (video_channel && audio_channel) {
        video_channel->setAudioChannel(audio_channel);
        video_channel->start();
        audio_channel->start();
    }

    // prepare 需要占用大量的资源，不允许占用主线程
    pthread_create(&pid_start, nullptr, start_onThread, this);
}

void LtPlayer::setRenderCallback(RenderCallback callback) {
    this->renderCallback = callback;
}

int LtPlayer::getDuration() {
    return duration;
}

void LtPlayer::setSeek(int progress) {


    // 健壮性判断
    if (progress < 0 || progress > duration) {

        return;
    }
    if (!audio_channel && !video_channel) {

        return;
    }
    if (!formatContext) {

        return;
    }

    // formatContext 多线程， av_seek_frame内部会对我们的 formatContext上下文的成员做处理，安全的问题
    // 互斥锁 保证多线程情况下安全

    pthread_mutex_lock(&seek_mutex);

    // FFmpeg 大部分单位 == 时间基AV_TIME_BASE
    /**
     * 1.formatContext 安全问题
     * 2.-1 代表默认情况，FFmpeg自动选择 音频 还是 视频 做 seek，  模糊：0视频  1音频
     * 3. AVSEEK_FLAG_ANY（老实） 直接精准到 拖动的位置，问题：如果不是关键帧，B帧 可能会造成 花屏情况
     *    AVSEEK_FLAG_BACKWARD（则优  8的位置 B帧 ， 找附件的关键帧 6，如果找不到他也会花屏）
     *    AVSEEK_FLAG_FRAME 找关键帧（非常不准确，可能会跳的太多），一般不会直接用，但是会配合用
     */
    int r = av_seek_frame(formatContext, -1, progress * AV_TIME_BASE, AVSEEK_FLAG_FRAME);
    if (r < 0) {
        // TODO 同学们自己去完成，给Java的回调
        return;
    }

    // TODO 如果你的视频，假设出了花屏，AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME， 缺点：慢一些
    // 有一点点冲突，后面再看 （则优  | 配合找关键帧）
    // av_seek_frame(formatContext, -1, progress * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);

    // 音视频正在播放，用户去 seek，我是不是应该停掉播放的数据  音频1frames 1packets，  视频1frames 1packets 队列

    // 这四个队列，还在工作中，让他们停下来， seek完成后，重新播放
    if (audio_channel) {
        audio_channel->packets.setWork(0);  // 队列不工作
        audio_channel->frames.setWork(0);  // 队列不工作
        audio_channel->packets.clear();
        audio_channel->frames.clear();
        audio_channel->packets.setWork(1); // 队列继续工作
        audio_channel->frames.setWork(1);  // 队列继续工作
    }

    if (video_channel) {
        video_channel->packets.setWork(0);  // 队列不工作
        video_channel->frames.setWork(0);  // 队列不工作
        video_channel->packets.clear();
        video_channel->frames.clear();
        video_channel->packets.setWork(1); // 队列继续工作
        video_channel->frames.setWork(1);  // 队列继续工作
    }

    pthread_mutex_unlock(&seek_mutex);


}

void *task_stop(void *args) {
    auto *player = static_cast<LtPlayer *>(args);
    player->stop_();
    return nullptr; // 必须返回，坑，错误很难找
}

void LtPlayer::stop_() {
    playing = false;
    pthread_join(pid_prepare, nullptr);
    pthread_join(pid_start, nullptr);

    // pid_prepare pid_start 就全部停止下来了  稳稳的停下来
    if (formatContext) {
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }
    DELETE(audio_channel);
    DELETE(video_channel);
    auto *ltPlayer = reinterpret_cast<LtPlayer *>(this);
    DELETE(ltPlayer)
}


void LtPlayer::stop() {
    // 只要用户关闭了，就不准你回调给Java成 start播放
    helper = nullptr;
    if (audio_channel) {
        audio_channel->jniCallbakcHelper = nullptr;
    }
    if (video_channel) {
        video_channel->jniCallbakcHelper = nullptr;
    }


    // 如果是直接释放 我们的 prepare_ start_ 线程，不能暴力释放 ，否则会有bug

    // 让他 稳稳的停下来

    // 我们要等这两个线程 稳稳的停下来后，我再释放DerryPlayer的所以工作
    // 由于我们要等 所以会ANR异常

    // 所以我们我们在开启一个 stop_线程 来等你 稳稳的停下来
    // 创建子线程
    pthread_create(&pid_stop, nullptr, task_stop, this);
}




