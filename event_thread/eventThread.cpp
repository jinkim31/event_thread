#include "eventThread.h"

std::vector<EventThread*> EventThread::ethreads;

EventThread::EventThread()
{
    mEventQueueSize = 1000;
    mIsSchedAttrAvailable = false;
    mIsLoopRunning = false;
    mLoopPeriod = 0;
    ethreads.push_back(this);
}

EventThread::~EventThread()
{
    /*
     * stop() should be called explicitly befor thread destruction.
     * crashes like following will occur:
     *
     * pure virtual method called
     * terminate called without an active exception
     *
     * stop() here in the destructor is here just in case and not a safe method for thread termination
     * since the derived class implementing vitual void task() is destructed prior to EventThread itself.
     */
    stop();
}

bool EventThread::checkLoopRunningSafe()
{
    std::unique_lock<std::mutex> lock(mMutexLoop);
    return mIsLoopRunning;
}

void EventThread::setLoopPeriod(uint64_t nsecPeriod)
{
    if(checkLoopRunningSafe()) return;
    mLoopPeriod = nsecPeriod;
}

void EventThread::setLoopFreq(double freq)
{
    if(checkLoopRunningSafe()) return;
    mLoopPeriod = NSEC_PER_SEC / freq;
}

void EventThread::start()
{
    if(checkLoopRunningSafe()) return;
    pthread_create(&mPthread, NULL, EventThread::threadEntryPoint, this);
}

void EventThread::stop()
{
    mMutexLoop.lock();
    mIsLoopRunning = false;
    mMutexLoop.unlock();
    void* ret;
    pthread_join(mPthread, &ret);
}

void EventThread::queueNewEvent(const std::function<void ()> &func)
{
    std::unique_lock<std::mutex> lock(mMutexEvent);
    if(mEventQueue.size() < mEventQueueSize) mEventQueue.push(func);
}

void *EventThread::threadEntryPoint(void *param)
{
    EventThread* ethreadPtr = (EventThread*)param;
    ethreadPtr->mTid = gettid();
    ethreadPtr->mPid = getpid();
    if(ethreadPtr->mIsSchedAttrAvailable) syscall(__NR_sched_setattr, 0, ethreadPtr->mSchedAttr, 0);
    //std::cout<<"starting new thread (pid:"<<ethreadPtr->mPid<<", tid:"<<ethreadPtr->mTid<<")"<<std::endl;
    clock_gettime(CLOCK_MONOTONIC, &(ethreadPtr->mNextTaskTime));
    timespecForward(&(ethreadPtr->mNextTaskTime), ethreadPtr->mLoopPeriod);
    ethreadPtr->runLoop();
    return nullptr;
}

void EventThread::handleQueuedEvents()
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

void EventThread::runLoop()
{
    std::unique_lock<std::mutex> lock(mMutexLoop);
    mIsLoopRunning = true;
    lock.unlock();

    while(checkLoopRunningSafe())
    {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &mNextTaskTime, NULL);
        timespecForward(&mNextTaskTime, mLoopPeriod);
        task();
        handleQueuedEvents();
    }
}

void EventThread::setSched(SchedAttr& attr)
{
    if(checkLoopRunningSafe()) return;
    attr.size = sizeof(SchedAttr);
    mSchedAttr = attr;
    mIsSchedAttrAvailable = true;
}

void EventThread::timespecForward(timespec* ts, int64_t nsecTime)
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
