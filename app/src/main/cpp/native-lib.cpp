#include <jni.h>
#include <string>
#include "LtPlayer.h"
#include "log4c.h"
#include <android/native_window_jni.h>

extern "C" {
#include <libavutil/avutil.h>
}

JavaVM *vm = nullptr;
ANativeWindow *aNativeWindow = nullptr;
pthread_mutex_t mutex = PTHREAD_COND_INITIALIZER;

jint JNI_OnLoad(JavaVM *javaVm, void *args) {
    ::vm = javaVm;
    return JNI_VERSION_1_6;
}

void renderCallback(uint8_t *src_data, int w, int h, int src_lineSize) {
    pthread_mutex_lock(&mutex);
    if (!aNativeWindow) {
        pthread_mutex_unlock(&mutex); // 出现了问题后，必须考虑到，释放锁，怕出现死锁问题
    }
    // 设置窗口的大小，各个属性
    ANativeWindow_setBuffersGeometry(aNativeWindow, w, h, WINDOW_FORMAT_RGBA_8888);

    // 他自己有个缓冲区 buffer
    ANativeWindow_Buffer window_buffer;

    // 如果我在渲染的时候，是被锁住的，那我就无法渲染，我需要释放 ，防止出现死锁
    if (ANativeWindow_lock(aNativeWindow, &window_buffer, nullptr)) {
        ANativeWindow_release(aNativeWindow);
        aNativeWindow = nullptr;

        pthread_mutex_unlock(&mutex); // 解锁，怕出现死锁
        return;
    }

    // TODO 开始真正的渲染，因为window没有被锁住了，就可以rgba数据 ----> 字节对齐 渲染
    // 填充[window_buffer]  画面就出来了  ==== 【目标 window_buffer】
    auto *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    int dst_linesize = window_buffer.stride * 4;
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst_data + i * dst_linesize, src_data + i * src_lineSize, dst_linesize);
    }

    // 数据刷新
    ANativeWindow_unlockAndPost(aNativeWindow); // 解锁后 并且刷新 window_buffer的数据显示画面

    pthread_mutex_unlock(&mutex);
}


extern "C"
JNIEXPORT jlong JNICALL
Java_com_lautung_ltplayer_LtPlayer_prepareNative(JNIEnv *env, jobject job,
                                                 jstring media_data_source) {
    const char *source = env->GetStringUTFChars(media_data_source, nullptr);
    auto *helper = new JNICallbakcHelper(vm, env, job);

    auto *player = new LtPlayer(source, helper);
    player->setRenderCallback(renderCallback);
    player->prepare();
    env->ReleaseStringUTFChars(media_data_source, source);
    return reinterpret_cast<jlong>(player);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_lautung_ltplayer_LtPlayer_startNative(JNIEnv *env, jobject thiz, jlong native_obj) {
    auto *player = reinterpret_cast<LtPlayer *>(native_obj);
    player->start();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_lautung_ltplayer_LtPlayer_stopNative(JNIEnv *env, jobject thiz, jlong native_obj) {
    // TODO: implement stopNative()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_lautung_ltplayer_LtPlayer_releaseNative(JNIEnv *env, jobject thiz, jlong native_obj) {
    // TODO: implement releaseNative()
}




extern "C"
JNIEXPORT void JNICALL
Java_com_lautung_ltplayer_LtPlayer_setSurfaceNative(JNIEnv *env, jobject thiz, jlong native_obj,
                                                    jobject surface) {
    pthread_mutex_lock(&mutex);

    if (aNativeWindow) {
        ANativeWindow_release(aNativeWindow);
        aNativeWindow = nullptr;
    }

    aNativeWindow = ANativeWindow_fromSurface(env, surface);

    pthread_mutex_unlock(&mutex);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_lautung_ltplayer_LtPlayer_getDurationNative(JNIEnv *env, jobject thiz, jlong native_obj) {
    auto *player = reinterpret_cast<LtPlayer *>(native_obj);
    if (player) {
        return player->getDuration();
    }
    return 0;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_lautung_ltplayer_LtPlayer_setSeekNative(JNIEnv *env, jobject thiz, jlong native_obj,
                                                 jint play_value) {
    auto *player = reinterpret_cast<LtPlayer *>(native_obj);
    if (player) {
        player->setSeek(play_value);
    }
}