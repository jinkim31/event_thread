#ifndef EVENTTHREAD_H
#define EVENTTHREAD_H

#include <pthread.h>
#include <iostream>
#include <mutex>
#include <functional>
#include <queue>
#include <vector>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/sched.h>
#include <sys/types.h>
#include <linux/kernel.h>

#define events public
#define NSEC_PER_SEC 1000000000

namespace ethr
{

class EventThread
{
public:
    enum class EventHandleScheme
    {
        AFTER_TASK,
        BEFORE_TASK,
        USER_EXPLICIT,
    };

    struct SchedAttr
    {
        __u32 size;
        __u32 sched_policy;
        __u64 sched_flags;
        __s32 sched_nice;
        __u32 sched_priority;
        __u64 sched_runtime;
        __u64 sched_deadline;
        __u64 sched_period;
    };

    EventThread();
    ~EventThread();
    void start();
    void stop();
    void setSched(SchedAttr& attr);
    void setLoopPeriod(uint64_t nsecPeriod);
    void setLoopFreq(double freq);
    void setEventHandleScheme(EventHandleScheme scheme);
    template<typename ObjPtr, typename FuncPtr, class... Args>
    static void callQueued(ObjPtr objPtr, FuncPtr funcPtr, Args... args);
    template<typename EThreadType, class... Args>
    static void callInterthread(void(EThreadType::* func)(Args...), Args... args);
protected:
    pid_t mPid, mTid;
    virtual void task()=0;
    void handleQueuedEvents();
private:
    pthread_t mPthread;
    std::mutex mMutexLoop, mMutexEvent;
    std::queue<std::function<void(void)>> mEventQueue;
    size_t mEventQueueSize;
    SchedAttr mSchedAttr;
    timespec mNextTaskTime;
    int64_t mLoopPeriod;
    bool mIsSchedAttrAvailable;
    bool mIsLoopRunning;
    EventHandleScheme mEventHandleScheme;
    bool checkLoopRunningSafe();
    void queueNewEvent(const std::function<void ()> &func);
    void runLoop();
    static void* threadEntryPoint(void* param);
    static void timespecForward(timespec* ts, int64_t nsecTime);
    static std::vector<EventThread*> ethreads;
};

}

template<typename ObjPtr, typename FuncPtr, class... Args>
void ethr::EventThread::callQueued(ObjPtr objPtr, FuncPtr funcPtr, Args... args)
{
    ((EventThread*)objPtr)->queueNewEvent(std::bind(funcPtr, objPtr, args...));
}

template<typename EThreadType, class... Args>
void ethr::EventThread::callInterthread(void(EThreadType::* func)(Args...), Args... args)
{
    for(const auto& ethreadPtr : EventThread::ethreads)
    {   if(typeid(*ethreadPtr) == typeid(EThreadType))
            callQueued((EThreadType*)ethreadPtr, func, args...);
    }
}

#endif
