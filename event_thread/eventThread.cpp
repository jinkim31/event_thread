#include "eventThread.h"

std::vector<EventThread*> EventThread::ethreads;

EventThread::EventThread()
{
    mEventQueueSize = 1000;
    ethreads.push_back(this);
}

EventThread::~EventThread()
{
    void* ret;
    pthread_join(mPthread, &ret);
}

void EventThread::start()
{
    pthread_create(&mPthread, NULL, EventThread::threadEntryPoint, this);
}

void EventThread::stop()
{
    std::unique_lock<std::mutex> lock(mMutexLoop);
    mIsLoopRunning = false;
    lock.unlock();
}

void EventThread::setLoopFreq(double freq)
{
    mLoopInterval = NSEC_PER_SEC / freq;
}

void EventThread::queueNewEvent(const std::function<void ()> &func)
{
    std::unique_lock<std::mutex> lock(mMutexEvent);
    if(mEventQueue.size() < mEventQueueSize) mEventQueue.push(func);
}

void *EventThread::threadEntryPoint(void *param)
{
    ((EventThread*)param)->runLoop();
    return nullptr;
}

void EventThread::handleQueuedEvents()
{
    auto isRunningSafe = [&]{std::unique_lock<std::mutex> lock(mMutexLoop); return mIsLoopRunning;};

    while(!mEventQueue.empty() && isRunningSafe())
    {
        // lock mutex and copy function at the front
        std::unique_lock<std::mutex> eventLock(mMutexEvent);
        auto func = mEventQueue.front();
        mEventQueue.pop();
        eventLock.unlock();

       // execute function
       func();
    }
}

void EventThread::runLoop()
{
    std::unique_lock<std::mutex> lock(mMutexLoop);
    mIsLoopRunning = true;
    lock.unlock();

    auto checkRunningSafe = [&]{
        std::unique_lock<std::mutex> lock(mMutexLoop);
        return mIsLoopRunning;
    };

    while(checkRunningSafe())
    {
        task();
        handleQueuedEvents();
    }
}
