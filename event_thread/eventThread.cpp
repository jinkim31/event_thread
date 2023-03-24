#include "eventThread.h"

std::vector<ethr::EventThread*> ethr::EventThread::ethreads;

ethr::EventThread::EventThread(const std::string& name)
{
    mName = name;
    mEventQueueSize = 1000;
    mIsLoopRunning = false;
    mEventHandleScheme = EventHandleScheme::AFTER_TASK;
    mTaskPeriod = std::chrono::high_resolution_clock::duration::zero();
    ethreads.push_back(this);
}

ethr::EventThread::~EventThread()
{
    /*
     * stop() should be called explicitly befor thread destruction.
     * otherwise crashes like following will occur:
     *
     * pure virtual method called
     * terminate called without an active exception
     *
     * stop() is here just in case and it's not safe to terminate a thread with it
     * since the derived class implementing vitual void task() is destructed prior to EventThread itself.
     */
    stop();
}

bool ethr::EventThread::checkLoopRunningSafe()
{
    std::unique_lock<std::mutex> lock(mMutexLoop);
    return mIsLoopRunning;
}

void ethr::EventThread::setName(const std::string& name)
{
    mName = name;
}

void ethr::EventThread::setLoopPeriod(std::chrono::duration<long long int, std::nano> period)
{
    if(checkLoopRunningSafe()) return;
    mTaskPeriod = period;
}

void ethr::EventThread::setLoopFreq(const unsigned int freq)
{
    if(checkLoopRunningSafe()) return;
    mTaskPeriod = std::chrono::seconds(1) / freq;
}

void ethr::EventThread::setEventHandleScheme(EventHandleScheme scheme)
{
    if(checkLoopRunningSafe()) return;
    mEventHandleScheme = scheme;
}

void ethr::EventThread::start(bool isMain)
{
    if(checkLoopRunningSafe()) return;

    mIsMainThread = isMain;

    if(isMain)
    {
        EventThread::threadEntryPoint(this);
    }
    else
    {
#ifdef ETHREAD_USE_PTHREAD
        pthread_create(&mThread, NULL, EventThread::threadEntryPoint, this);
#else
        mThread = std::thread(EventThread::threadEntryPoint, this);
#endif
    }
}

void ethr::EventThread::stop()
{
    if(!checkLoopRunningSafe()) return;

    mMutexLoop.lock();
    mIsLoopRunning = false;
    mMutexLoop.unlock();
    if(!mIsMainThread)
    {
#ifdef ETHREAD_USE_PTHREAD
        void* ret;
        pthread_join(mThread, &ret);
#else
        if(mThread.joinable())
            mThread.join();
#endif
    }
}

void ethr::EventThread::queueNewEvent(const std::function<void ()> &func)
{
    std::unique_lock<std::mutex> lock(mMutexEvent);
    if(mEventQueue.size() < mEventQueueSize) mEventQueue.push(func);
}

void *ethr::EventThread::threadEntryPoint(void *param)
{
    EventThread* ethreadPtr = (EventThread*)param;
    //ethreadPtr->mTid = gettid();
    //ethreadPtr->mPid = getpid();
    ethreadPtr->mNextTaskTime = std::chrono::high_resolution_clock::now() + ethreadPtr->mTaskPeriod;
    ethreadPtr->runLoop();
    return nullptr;
}

void ethr::EventThread::handleQueuedEvents()
{
    while(!mEventQueue.empty())
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

void ethr::EventThread::runLoop()
{
    std::unique_lock<std::mutex> lock(mMutexLoop);
    mIsLoopRunning = true;
    lock.unlock();

    onStart();

    while(checkLoopRunningSafe())
    {
        while(std::chrono::high_resolution_clock::now() < mNextTaskTime);
        mNextTaskTime += mTaskPeriod;

        switch(mEventHandleScheme)
        {
        case EventHandleScheme::AFTER_TASK:
        {
            task();
            handleQueuedEvents();
            break;
        }
        case EventHandleScheme::BEFORE_TASK:
        {
            handleQueuedEvents();
            task();
            break;
        }
        case EventHandleScheme::USER_CONTROLLED:
        {
            task();
            break;
        }
        }
    }

    onTerminate();
}
