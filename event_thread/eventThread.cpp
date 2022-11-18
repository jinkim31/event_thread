#include "eventThread.h"

std::vector<ethr::EventThread*> ethr::EventThread::ethreads;

ethr::EventThread::EventThread(const std::string& name)
{
    mName = name;
    mEventQueueSize = 1000;
    mIsSchedAttrAvailable = false;
    mIsLoopRunning = false;
    mEventHandleScheme = EventHandleScheme::AFTER_TASK;
    mLoopPeriod = 0;
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

void ethr::EventThread::setLoopPeriod(uint64_t nsecPeriod)
{
    if(checkLoopRunningSafe()) return;
    mLoopPeriod = nsecPeriod;
}

void ethr::EventThread::setLoopFreq(double freq)
{
    if(checkLoopRunningSafe()) return;
    mLoopPeriod = NSEC_PER_SEC / freq;
}

void ethr::EventThread::setEventHandleScheme(EventHandleScheme scheme)
{
    if(checkLoopRunningSafe()) return;
    mEventHandleScheme = scheme;
}

void ethr::EventThread::start()
{
    if(checkLoopRunningSafe()) return;
    pthread_create(&mPthread, NULL, EventThread::threadEntryPoint, this);
}

void ethr::EventThread::stop()
{
    mMutexLoop.lock();
    mIsLoopRunning = false;
    mMutexLoop.unlock();
    void* ret;
    pthread_join(mPthread, &ret);
}

void ethr::EventThread::queueNewEvent(const std::function<void ()> &func)
{
    if(!checkLoopRunningSafe()) return;
    std::unique_lock<std::mutex> lock(mMutexEvent);
    if(mEventQueue.size() < mEventQueueSize) mEventQueue.push(func);
}

void *ethr::EventThread::threadEntryPoint(void *param)
{
    EventThread* ethreadPtr = (EventThread*)param;
    ethreadPtr->mTid = gettid();
    ethreadPtr->mPid = getpid();
    if(ethreadPtr->mIsSchedAttrAvailable) syscall(__NR_sched_setattr, 0, ethreadPtr->mSchedAttr, 0);
    clock_gettime(CLOCK_MONOTONIC, &(ethreadPtr->mNextTaskTime));
    timespecForward(&(ethreadPtr->mNextTaskTime), ethreadPtr->mLoopPeriod);
    ethreadPtr->runLoop();
    return nullptr;
}

void ethr::EventThread::handleQueuedEvents()
{
    while(!mEventQueue.empty() && checkLoopRunningSafe())
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
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &mNextTaskTime, NULL);
        timespecForward(&mNextTaskTime, mLoopPeriod);

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
        case EventHandleScheme::USER_EXPLICIT:
        {
            task();
            break;
        }
        }
    }
}

void ethr::EventThread::setSched(SchedAttr& attr)
{
    if(checkLoopRunningSafe()) return;
    attr.size = sizeof(SchedAttr);
    mSchedAttr = attr;
    mIsSchedAttrAvailable = true;
}

void ethr::EventThread::timespecForward(timespec* ts, int64_t nsecTime)
{
    int64_t sec, nsec;

    nsec = nsecTime % NSEC_PER_SEC;
    sec = (nsecTime - nsec) / NSEC_PER_SEC;
    ts->tv_sec += sec;
    ts->tv_nsec += nsec;
    if (ts->tv_nsec > NSEC_PER_SEC)
    {
        nsec = ts->tv_nsec % NSEC_PER_SEC;
        ts->tv_sec += (ts->tv_nsec - nsec) / NSEC_PER_SEC;
        ts->tv_nsec = nsec;
    }
}
