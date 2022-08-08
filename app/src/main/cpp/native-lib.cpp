#include <jni.h>
#include <string>
#include "LtPlayer.h"

JavaVM *vm = nullptr;

jint JNI_OnLoad(JavaVM *vm, void *args) {
    ::vm = vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_lautung_ltplayer_LtPlayer_prepareNative(JNIEnv *env, jobject job,
                                                 jstring media_data_source) {
    const char *source = env->GetStringUTFChars(media_data_source, nullptr);
    auto *helper = new JNICallbakcHelper(vm, env, job);

    auto *player = new LtPlayer(source, helper);
    player->prepare();
    return reinterpret_cast<jlong>(player);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_lautung_ltplayer_LtPlayer_startNative(JNIEnv *env, jobject thiz, jobject native_obj) {
    auto *player = reinterpret_cast<LtPlayer *>(native_obj);
    player->start();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_lautung_ltplayer_LtPlayer_stopNative(JNIEnv *env, jobject thiz, jobject native_obj) {
    // TODO: implement stopNative()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_lautung_ltplayer_LtPlayer_releaseNative(JNIEnv *env, jobject thiz, jobject native_obj) {
    // TODO: implement releaseNative()
}