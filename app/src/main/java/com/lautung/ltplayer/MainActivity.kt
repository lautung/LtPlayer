package com.lautung.ltplayer

import android.Manifest
import android.annotation.SuppressLint
import android.content.pm.PackageManager
import android.graphics.Color
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.SurfaceView
import android.view.View
import android.view.WindowManager
import android.widget.SeekBar
import android.widget.SeekBar.OnSeekBarChangeListener
import android.widget.TextView
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.lautung.ltplayer.databinding.ActivityMainBinding
import java.io.File

class MainActivity : AppCompatActivity(), OnSeekBarChangeListener {

    private lateinit var binding: ActivityMainBinding

    private lateinit var player: LtPlayer
    private lateinit var tvState: TextView
    private lateinit var surfaceView: SurfaceView

    private lateinit var seekBar: SeekBar
    private lateinit var tvTime: TextView
    private var isTouch = false
    private var duration = 0


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
        )

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        tvState = binding.tvState
        surfaceView = binding.surfaceView
        seekBar = binding.seekBar
        tvTime = binding.tvTime
        seekBar.setOnSeekBarChangeListener(this)
        player = LtPlayer(this)
        player.setSurfaceView(surfaceView)
        player.setMediaDataSource(
            File(Environment.getExternalStorageDirectory(), "demo.mp4").absolutePath
        )

        // 准备成功的回调处    <----  C++ 子线程调用的
        player.setOnPreparedListener(object : LtPlayer.OnPreparedListener {
            override fun onPrepared() {

                //                    如果是直播duration为0，非直播即视频，duration不为0.
                duration = player.duration

                runOnUiThread {
                    if (duration != 0) {
                        tvTime.text =
                            String.format("00:00/%s:%s", getMinutes(duration), getSeconds(duration))
                        tvTime.visibility = View.VISIBLE
                        seekBar.visibility = View.VISIBLE
                    } else {
                        tvTime.visibility = View.GONE
                        seekBar.visibility = View.GONE
                    }

                    tvState.setTextColor(Color.GREEN) // 绿色
                    tvState.text = "恭喜init初始化成功"
                }
                player.start() // 调用C++ 开始播放
            }
        })

        player.setOnErrorListener(object : LtPlayer.OnErrorListener {

            @SuppressLint("SetTextI18n")
            override fun onError(errorCode: String?) {
                runOnUiThread { // Toast.makeText(MainActivity.this, "出错了，错误详情是:" + errorInfo, Toast.LENGTH_SHORT).show();
                    tvState.setTextColor(Color.RED) // 红色
                    tvState.text = errorCode
                }
            }
        })

        player.setOnOnProgressListener(object : LtPlayer.OnProgressListener {
            override fun onProgress(progress: Int) {
                // 【如果是人为拖动的，不能干预我们计算】 否则会混乱
                if (!isTouch) {

                    // C++层是异步线程调用上来的，小心，UI
                    runOnUiThread {
                        if (duration != 0) {
                            // TODO 播放信息 动起来
                            // progress:C++层 ffmpeg获取的当前播放【时间（单位是秒 80秒都有，肯定不符合界面的显示） -> 1分20秒】
                            tvTime.text = String.format(
                                "%s:%s/%s:%s",
                                getMinutes(progress),
                                getSeconds(progress),
                                getMinutes(duration),
                                getSeconds(duration)
                            )

                            // TODO 拖动条 动起来 seekBar相对于总时长的百分比
                            // progress == C++层的 音频时间搓  ----> seekBar的百分比
                            // seekBar.setProgress(progress * 100 / duration 以秒计算seekBar相对总时长的百分比);
                            seekBar.progress = progress * 100 / duration
                        }
                    }
                }
            }

        })

        // 动态 6.0及以上的 申请权限
        checkPermission()

    }

    // TODO ===================================================================== 下面代码是 6.0及以上 动态权限申请 区域 start
    private var permissions =
        arrayOf<String>(Manifest.permission.WRITE_EXTERNAL_STORAGE) // 如果要申请多个动态权限，此处可以写多个

    var mPermissionList: MutableList<String> = ArrayList()

    private val PERMISSION_REQUEST = 1


    // 检查权限
    private fun checkPermission() {
        mPermissionList.clear()

        // 判断哪些权限未授予
        for (permission in permissions) {
            if (ContextCompat.checkSelfPermission(
                    this,
                    permission
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                mPermissionList.add(permission)
            }
        }

        // 判断是否为空
        if (mPermissionList.isEmpty()) { // 未授予的权限为空，表示都授予了
        } else {
            //请求权限方法
            val permissions = mPermissionList.toTypedArray() //将List转为数组
            ActivityCompat.requestPermissions(this, permissions, PERMISSION_REQUEST)
        }
    }

    /**
     * 响应授权
     * 这里不管用户是否拒绝，都进入首页，不再重复申请权限
     */
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String?>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        when (requestCode) {
            PERMISSION_REQUEST -> {}
            else -> super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        }
    }

    // TODO ===================================================================== 下面代码是 6.0及以上 动态权限申请 区域 end

    override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
        if (fromUser) {
            // progress 是进度条的进度 （0 - 100） ------>   秒 分 的效果
            tvTime.text = String.format(
                "%s:%s/%s:%s",
                getMinutes(progress * duration / 100),
                getSeconds(progress * duration / 100),
                getMinutes(duration),
                getSeconds(duration)
            )
        }
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {
        isTouch = true
    }

    override fun onStopTrackingTouch(seekBar: SeekBar?) {
        isTouch = false
        seekBar?.apply {
            val cprogress= duration * this.progress / 100
            player.setSeek(cprogress)
        }
    }


    private fun getMinutes(duration: Int): String { // 给我一个duration，转换成xxx分钟
        val minutes = duration / 60
        return if (minutes <= 9) {
            "0$minutes"
        } else "" + minutes
    }

    // 119 ---> 60 59
    private fun getSeconds(duration: Int): String { // 给我一个duration，转换成xxx秒
        val seconds = duration % 60
        return if (seconds <= 9) {
            "0$seconds"
        } else "" + seconds
    }

}