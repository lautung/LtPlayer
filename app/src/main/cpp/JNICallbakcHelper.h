#ifndef DERRYPLAYER_JNICALLBAKCHELPER_H
#define DERRYPLAYER_JNICALLBAKCHELPER_H


extern "C" {
#include <jni.h>
#include "util.h"
};


class JNICallbakcHelper {

private:
    JavaVM *vm = 0;   // 只有他 才能 跨越线程
    JNIEnv *env = 0; // 主线程 调用 Kotlin 用的
    jobject job; // 为了更好的寻找到 DerryPlayer.kt实例
    jmethodID jmd_prepared;
    jmethodID jmd_onError;

public:
    JNICallbakcHelper(JavaVM *vm, JNIEnv *env, jobject job);

    virtual ~JNICallbakcHelper();

    void onPrepared(int thread_mode);
    void onError(int thread_mode, int error_code, char * ffmpegError);


};

#endif //DERRYPLAYER_JNICALLBAKCHELPER_H
