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
    typedef void (*SyncCallback)(queue <T> &);

public:
    queue <T> queue;
    pthread_mutex_t mutex{}; // 互斥锁 安全
    pthread_cond_t cond{}; // 等待 和 唤醒
    bool work = false; // 标记队列是否工作
    ReleaseCallback releaseCallback;
    SyncCallback syncCallback;

    SafeQueue() {
        pthread_mutex_init(&mutex, nullptr); // 初始化互斥锁
        pthread_cond_init(&cond, nullptr); // 初始化条件变量
    }

    ~SafeQueue() {
        pthread_mutex_destroy(&mutex); // 释放互斥锁
        pthread_cond_destroy(&cond); // 释放条件变量
    };

    /**
    * 入队 [ AVPacket *  压缩包]  [ AVFrame * 原始包]
    */
    void insertToQueue(T value) {
        pthread_mutex_lock(&mutex); // 多线程的访问（先锁住）

        if (work) {
            // 工作状态
            queue.push(value);
            pthread_cond_signal(&cond); // 当插入数据包 进队列后，要发出通知唤醒
        } else {
            // 非工作状态，释放value, 不知道如何释放， T类型不明确，我没有办法释放（让外界释放）
            if (releaseCallback) {
                releaseCallback(&value); // 让外界释放我们的 value
            }
        }

        pthread_mutex_unlock(&mutex); // 多线程的访问（要解锁）
    };

    // get T  如果没有数据，我就睡觉
    /**
    *  出队 [ AVPacket *  压缩包]  [ AVFrame * 原始包]
    */
    int getQueueAndDel(T &value) { // 获取队列数据后，并且 删除 ？ 为了不混乱
        int ret = 0; // 默认是false

        pthread_mutex_lock(&mutex); // 多线程的访问（先锁住）

        // 如果是if，只执行一次，执行一次 就不明确
        while (work && queue.empty()) {
            // 如果是工作专题 并且 队列里面没有数据，我就阻塞这这里睡觉
            pthread_cond_wait(&cond, &mutex); // 没有数据就睡觉（C++线程的内容）
        }

        if (!queue.empty()) { // 如果队列里面有数据，就进入此if
            // 取出队列的数据包 给外界，并删除队列数据包
            value = queue.front();
            queue.pop(); // 删除队列中的数据
            ret = 1;
        }

        pthread_mutex_unlock(&mutex); // 多线程的访问（要解锁）

        return ret;
    };

    // Activity onDestroy  ----> setWork(0 false);
    /**
    * 设置工作状态，设置队列是否工作
    * @param i
    */
    void setWork(bool b) {
        pthread_mutex_lock(&mutex); // 多线程的访问（先锁住）

        this->work = b;

        // 每次设置状态后，就去唤醒下，有没有阻塞睡觉的地方[严谨处理]
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex); // 多线程的访问（要解锁）
    };

    int empty() {
        return queue.empty();
    }

    int size() {
        return queue.size();
    };

    /**
     * 清空队列中所有的数据，循环一个一个的删除
     */
    void clear() {
        pthread_mutex_lock(&mutex); // 多线程的访问（先锁住）

        unsigned int size = queue.size();

        for (int i = 0; i < size; ++i) {
            //循环释放队列中的数据
            T value = queue.front();
            if (releaseCallback) {
                releaseCallback(&value); // 让外界释放我们的 value
            }
            queue.pop(); // 删除队列中的数据，让队列为0
        }

        pthread_mutex_unlock(&mutex); // 多线程的访问（要解锁）
    };

    /**
    * 设置此函数指针的回调，让外界去释放  value T
    * @param callback
    */
    void setReleaseCallback(ReleaseCallback callback) {
        this->releaseCallback = callback;
    };

    void setSyncCallback(SyncCallback callback){
        this->syncCallback = callback;
    }

    void sync(){
        pthread_mutex_lock(&mutex);
        syncCallback(queue);

        pthread_mutex_unlock(&mutex);
    }
};


#endif //LTPLAYER_SAFEQUEUE_H
