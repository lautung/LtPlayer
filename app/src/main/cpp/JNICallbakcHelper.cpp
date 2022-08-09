#include "JNICallbakcHelper.h"

JNICallbakcHelper::JNICallbakcHelper(JavaVM *vm, JNIEnv *env, jobject job) {
    this->vm = vm;
    this->env = env;
    // this->job = job; //jobject不能跨越线程，不能跨越函数，必须全局引用
    this->job = env->NewGlobalRef(job); // 提示全局引用 解决

    jclass playerKTClass = env->GetObjectClass(job);
    jmd_prepared = env->GetMethodID(playerKTClass, "onPrepared", "()V");
    jmd_onError=env->GetMethodID(playerKTClass,"onError","(ILjava/lang/String;)V");
}

JNICallbakcHelper::~JNICallbakcHelper() {
    vm = nullptr;
    env->DeleteGlobalRef(job); // 释放全局引用
    job = nullptr;
    env = nullptr;
}

void JNICallbakcHelper::onPrepared(int thread_mode) {
    if (thread_mode == THREAD_MAIN) {
        // 主线程
        env->CallVoidMethod(job, jmd_prepared);
    } else if (thread_mode == THREAD_CHILD) {
        // 子线程 env也不可以跨线程吧 对的   全新的env   子线程 必须用 JavaVM 子线程中附加出来的新 子线程专用env
        JNIEnv * env_child;
        vm->AttachCurrentThread(&env_child, nullptr);
        env_child->CallVoidMethod(job, jmd_prepared);
        vm->DetachCurrentThread();
    }
}

void JNICallbakcHelper::onError(int thread_mode, int error_code, char *ffmpegError) {
    if (thread_mode == THREAD_MAIN) {
        //主线程
        jstring ffmpegError_ = env->NewStringUTF(ffmpegError);
        env->CallVoidMethod(job, jmd_onError, error_code, ffmpegError_);
    } else {
        //子线程
        //当前子线程的 JNIEnv
        JNIEnv *env_child;
        vm->AttachCurrentThread(&env_child, nullptr);
        jstring ffmpegError_ = env_child->NewStringUTF(ffmpegError);
        env_child->CallVoidMethod(job, jmd_onError, error_code, ffmpegError_);
        vm->DetachCurrentThread();
    }

}


