#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <mutex>
#include <queue>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue(){};

    void push(T value)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mQueueData.emplace(value);
        mCv.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mMutex);
        mCv.wait(lk,[this]{return !mQueueData.empty();});
        value = mQueueData.front();
    }

    void wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mMutex);
        mCv.wait(lk,[this]{return !mQueueData.empty();});
        mQueueData.pop();
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lk(mMutex);
        return mQueueData.empty();
    }

    int size()
    {
        std::lock_guard<std::mutex> lk(mMutex);
        return (int)mQueueData.size();
    }

private:
    std::mutex mMutex;
    std::queue<T> mQueueData;
    std::condition_variable mCv;
};

#endif // THREADSAFEQUEUE_H
