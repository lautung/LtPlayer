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
import android.view.WindowManager
import android.widget.TextView
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.lautung.ltplayer.databinding.ActivityMainBinding
import java.io.File

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    private lateinit var player: LtPlayer
    private lateinit var tvState: TextView
    private lateinit var surfaceView: SurfaceView

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
        player = LtPlayer(this)
        player.setSurfaceView(surfaceView)
        player.setMediaDataSource(
            File(getExternalFilesDir(""), "demo.mp4").absolutePath
        )

        // 准备成功的回调处    <----  C++ 子线程调用的
        player.setOnPreparedListener(object : LtPlayer.OnPreparedListener {
            override fun onPrepared() {
                runOnUiThread {
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
                    tvState?.setTextColor(Color.RED) // 红色
                    tvState?.text = errorCode
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

}