package com.lautung.ltplayer

import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.OnLifecycleEvent

class LtPlayer(lifecycleOwner: LifecycleOwner) : SurfaceHolder.Callback, LifecycleObserver {

    init {
        lifecycleOwner.lifecycle.addObserver(this)
    }

    companion object {
        // Used to load the 'ltplayer' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }


        // 错误代码 ================ 如下
        // 打不开视频
        // #define FFMPEG_CAN_NOT_OPEN_URL 1
        const val FFMPEG_CAN_NOT_OPEN_URL = 1

        // 找不到流媒体
        // #define FFMPEG_CAN_NOT_FIND_STREAMS 2
        const val FFMPEG_CAN_NOT_FIND_STREAMS = 2

        // 找不到解码器
        // #define FFMPEG_FIND_DECODER_FAIL 3
        const val FFMPEG_FIND_DECODER_FAIL = 3

        // 无法根据解码器创建上下文
        // #define FFMPEG_ALLOC_CODEC_CONTEXT_FAIL 4
        const val FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = 4

        //  根据流信息 配置上下文参数失败
        // #define FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL 6
        const val FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = 6

        // 打开解码器失败
        // #define FFMPEG_OPEN_DECODER_FAIL 7
        const val FFMPEG_OPEN_DECODER_FAIL = 7

        // 没有音视频
        // #define FFMPEG_NOMEDIA 8
        const val FFMPEG_NOMEDIA = 8
        /*  end */

    }

    private var onPreparedListener: OnPreparedListener? = null // C++层准备情况的接口

    private var onErrorListener: OnErrorListener? = null

    private var surfaceHolder: SurfaceHolder? = null

    private var nativeObj: Long? = null

    private var mediaDataSource: String = ""

    fun setMediaDataSource(mediaDataSource: String) {
        this.mediaDataSource = mediaDataSource
    }

    fun setSurfaceView(surfaceView: SurfaceView) {
        surfaceHolder?.removeCallback(this)
        surfaceHolder = surfaceView.holder
        surfaceHolder?.addCallback(this)
    }


    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    fun prepare() {
        nativeObj = prepareNative(mediaDataSource)
    }

    fun start() {
        nativeObj?.let { startNative(it) }
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    fun stop() {
        nativeObj?.let { stopNative(it) }
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    fun release() {
        nativeObj?.let { releaseNative(it) }
    }

    // 写一个函数，给C++调用
    /**
     * 给jni反射调用的  准备成功
     */
    fun onPrepared() {
        onPreparedListener?.onPrepared()
    }


    /**
     * 给jni反射调用的  回调错误
     */
    fun onError(errorCode: Int, ffmpegError: String) {
        val title = "\nFFmpeg给出的错误如下:\n"
        onErrorListener?.apply {
            var msg: String? = null
            when (errorCode) {
                FFMPEG_CAN_NOT_OPEN_URL -> msg = "打不开视频$title$ffmpegError"
                FFMPEG_CAN_NOT_FIND_STREAMS -> msg = "找不到流媒体$title$ffmpegError"
                FFMPEG_FIND_DECODER_FAIL -> msg = "找不到解码器$title$ffmpegError"
                FFMPEG_ALLOC_CODEC_CONTEXT_FAIL -> msg = "无法根据解码器创建上下文$title$ffmpegError"
                FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL -> msg =
                    "根据流信息 配置上下文参数失败$title$ffmpegError"
                FFMPEG_OPEN_DECODER_FAIL -> msg = "打开解码器失败$title$ffmpegError"
                FFMPEG_NOMEDIA -> msg = "没有音视频$title$ffmpegError"
            }
            onError(msg)
        }
    }


    /**
     * 准备OK的监听接口
     */
    interface OnPreparedListener {
        fun onPrepared()
    }

    /**
     * 准备OK的监听接口
     */
    interface OnErrorListener {
        fun onError(errorCode: String?)
    }

    /**
     * 设置准备OK的监听接口
     */
    fun setOnPreparedListener(onPreparedListener: OnPreparedListener) {
        this.onPreparedListener = onPreparedListener
    }

    fun setOnErrorListener(onErrorListener: OnErrorListener) {
        this.onErrorListener = onErrorListener
    }

    private external fun releaseNative(nativeObj: Long)

    private external fun startNative(nativeObj: Long)

    private external fun stopNative(nativeObj: Long)

    private external fun prepareNative(mediaDataSource: String): Long

    private external fun setSurfaceNative(nativeObj: Long, surface: Surface)


    override fun surfaceCreated(p0: SurfaceHolder) {}

    override fun surfaceChanged(p0: SurfaceHolder, p1: Int, p2: Int, p3: Int) {
        surfaceHolder?.surface?.let { nativeObj?.let { it1 -> setSurfaceNative(it1, it) } }
    }


    override fun surfaceDestroyed(p0: SurfaceHolder) {}

}