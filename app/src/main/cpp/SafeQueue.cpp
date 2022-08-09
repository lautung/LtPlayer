//
// Created by lautung on 2022/8/9.
//

#include "SafeQueue.h"

template<typename T>
SafeQueue<T>::SafeQueue() {
    pthread_mutex_init(&mutex, nullptr); // 初始化互斥锁
    pthread_cond_init(&cond, nullptr); // 初始化条件变量
}

template<typename T>
SafeQueue<T>::~SafeQueue() {
    pthread_mutex_destroy(&mutex); // 释放互斥锁
    pthread_cond_destroy(&cond); // 释放条件变量
}

template<typename T>
void SafeQueue<T>::insertToQueue(T value) {
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
}

template<typename T>
int SafeQueue<T>::getQueueAndDel(T &value) { // 获取队列数据后，并且 删除 ？ 为了不混乱
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
}


template<typename T>
void SafeQueue<T>::setWork(int i) {
    pthread_mutex_lock(&mutex); // 多线程的访问（先锁住）

    this->work = i;

    // 每次设置状态后，就去唤醒下，有没有阻塞睡觉的地方[严谨处理]
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&mutex); // 多线程的访问（要解锁）
}

template<typename T>
int SafeQueue<T>::empty() {
    return queue.empty();
}

template<typename T>
int SafeQueue<T>::size() {
    return queue.size();
}

template<typename T>
void SafeQueue<T>::clear() {
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
}

template<typename T>
void SafeQueue<T>::setReleaseCallback(SafeQueue::ReleaseCallback callback) {
    this->releaseCallback = callback;
}






