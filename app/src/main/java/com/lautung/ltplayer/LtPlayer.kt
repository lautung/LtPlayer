package com.lautung.ltplayer

import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.OnLifecycleEvent

class LtPlayer(lifecycleOwner: LifecycleOwner) : LifecycleObserver {

    init {
        lifecycleOwner.lifecycle.addObserver(this)
    }

    companion object {
        // Used to load the 'ltplayer' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }

    private var onPreparedListener : OnPreparedListener? = null // C++层准备情况的接口

    private var nativeObj: Long? = null

    private var mediaDataSource: String = ""

    fun setMediaDataSource(mediaDataSource: String) {
        this.mediaDataSource = mediaDataSource
    }


    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    fun prepare() {
        nativeObj = prepareNative(mediaDataSource)
    }

    fun start() {
        startNative(nativeObj)
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    fun stop() {
        stopNative(nativeObj)
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    fun release() {
        releaseNative(nativeObj)
    }

    // 写一个函数，给C++调用
    /**
     * 给jni反射调用的  准备成功
     */
    fun onPrepared() {
        if (onPreparedListener != null) {
            onPreparedListener!!.onPrepared()
        }
    }


    /**
     * 准备OK的监听接口
     */
    interface OnPreparedListener {
        fun onPrepared()
    }

    /**
     * 设置准备OK的监听接口
     */
    fun setOnPreparedListener(onPreparedListener: OnPreparedListener?) {
        this.onPreparedListener = onPreparedListener
    }

    private external fun releaseNative(nativeObj: Long?)

    private external fun startNative(nativeObj: Long?)

    private external fun stopNative(nativeObj: Long?)

    private external fun prepareNative(mediaDataSource: String): Long
}