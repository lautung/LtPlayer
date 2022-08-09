//
// Created by lautung on 2022/8/9.
//

#ifndef LTPLAYER_SAFEQUEUE_H
#define LTPLAYER_SAFEQUEUE_H

#include <queue>
#include <pthread.h>

using namespace std;

template<typename T> // 泛型：存放任意类型
class SafeQueue {
private:
    typedef void (*ReleaseCallback)(T *); // 函数指针定义 做回调 用来释放T里面的内容的

public:
    queue<T> queue;
    pthread_mutex_t mutex{}; // 互斥锁 安全
    pthread_cond_t cond{}; // 等待 和 唤醒
    int work{}; // 标记队列是否工作
    ReleaseCallback releaseCallback;

    SafeQueue();

    ~SafeQueue();

    /**
    * 入队 [ AVPacket *  压缩包]  [ AVFrame * 原始包]
    */
    void insertToQueue(T value);

    // get T  如果没有数据，我就睡觉
    /**
    *  出队 [ AVPacket *  压缩包]  [ AVFrame * 原始包]
    */
    int getQueueAndDel(T &value) ;

    // Activity onDestroy  ----> setWork(0 false);
    /**
    * 设置工作状态，设置队列是否工作
    * @param i
    */
    void setWork(int i) ;

    int empty();

    int size();

    /**
     * 清空队列中所有的数据，循环一个一个的删除
     */
    void clear();

    /**
    * 设置此函数指针的回调，让外界去释放  value T
    * @param callback
    */
    void setReleaseCallback(ReleaseCallback callback);
};



#endif //LTPLAYER_SAFEQUEUE_H
