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
#include <memory>
#include "sharedResource.h"

#define events public
#define NSEC_PER_SEC 1000000000


namespace ethr
{

/* forward dclr */
template<typename T>
class ThreadRef;

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

    EventThread(const std::string& name="");

    ~EventThread();

    /**
     * @brief Start thread.
     * 
     */
    void start();

    /**
     * @brief Stop thread. This should be called before EventThread destruction.
     * 
     */
    void stop();

    void setName(const std::string& name);
    /**
     * @brief Set the sched attributes.
     * 
     * @param attr 
     */
    void setSched(SchedAttr& attr);

    /**
     * @brief Set the loop period in nanoseconds.
     * 
     * @param nsecPeriod 
     */
    void setLoopPeriod(uint64_t nsecPeriod);

    /**
     * @brief Set the loop frequency in Hz.
     * 
     * @param freq 
     */
    void setLoopFreq(double freq);

    /**
     * @brief Set the event handling scheme. 
     * 
     * @param scheme 
     *  AFTER_TASK: handles event after task() execution
     *  BEFORE_TASK: handles event before task() execution
     *  USER_EXPLICIT: event handling is triggered when user explicitly calls handleQueuedEvents()
     */
    void setEventHandleScheme(EventHandleScheme scheme);

    /**
     * @brief Manipulate shared resource.
     * 
     * @tparam SharedResourceType 
     * @param manipulator std::function<void(SharedResourceType&)> type functor used to manipulate shared resource.
     */
    template<typename SharedResourceType>
    void manipulateSharedResource(const std::function<void(SharedResourceType&)>& manipulator);

    /**
     * @brief Add new event to thread event queue.
     * 
     * @tparam ObjPtr
     * @tparam FuncPtr
     * @tparam Args
     * @param objPtr ObjPtr EventThread object pointer to add event to
     * @param funcPtr event function pointer
     * @param args event function pointer arguments
     */
    template<typename ObjPtr, typename FuncPtr, class... Args>
    static void callQueued(ObjPtr objPtr, FuncPtr funcPtr, Args... args);

    /**
     * @brief Call interthread events. Several events may be called if multiple instance of EthreadType exists.
     * 
     * @tparam EthreadType 
     * @tparam Args 
     * @param func EventThread event function pointer
     * @param args EventThread event function arguments
     */
    template<typename EthreadType, class... Args>
    static void callInterthread(void(EthreadType::* func)(Args...), Args... args);

    /**
     * @brief Call interthread events of specific thread.
     * 
     * @tparam EthreadType 
     * @tparam Args 
     * @param ref ThreadRef of desired EventThread
     * @param func EventThread event function pointer
     * @param args EventThread event function arguments
     */
    template<typename EthreadType, class... Args>
    static void callInterthread(ThreadRef<EthreadType>& ref, void(EthreadType::* func)(Args...), Args... args);

protected:
    virtual void task()=0;      // pure virtual function that runs in the loop

    virtual void onStart()=0;   // pure virtual function that runs once when the thread starts

    void handleQueuedEvents();

    template<typename SharedResourceType>
    void makeSharedResource();

    template<typename EthreadType>
    static bool findThread(ThreadRef<EthreadType>& ref, const std::string& name);

private:
    std::string mName;
    pid_t mPid, mTid;
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
    std::unique_ptr<ISharedResource> mISharedResource;

    bool checkLoopRunningSafe();

    void queueNewEvent(const std::function<void ()> &func);

    void runLoop();

    static void* threadEntryPoint(void* param);

    static void timespecForward(timespec* ts, int64_t nsecTime);

    static std::vector<EventThread*> ethreads;
};

template <typename EthreadType>
class ThreadRef
{
public:
    ThreadRef(){}
    template<typename SharedResourceType>
    void manipulateSharedResource(const std::function<void(SharedResourceType&)>& manipulator)
    {
        ((EthreadType*)mRef)->manipulateSharedResource(manipulator);
    }
private:
    EthreadType* mRef;
friend ethr::EventThread;
};

}

template<typename EthreadType>
bool ethr::EventThread::findThread(ThreadRef<EthreadType>& ref, const std::string& name)
{
    for(const auto& ethreadPtr : EventThread::ethreads)
    {   if(typeid(*ethreadPtr) == typeid(EthreadType) && ethreadPtr->mName == name)
        {
                ref.mRef = (EthreadType*)ethreadPtr;
                return true;
        }
    }
    return false;
}

template<typename SharedResourceType>
void ethr::EventThread::makeSharedResource()
{
    mISharedResource = std::make_unique<SharedResource<SharedResourceType>>();
}
template<typename SharedResourceType>
void ethr::EventThread::manipulateSharedResource(const std::function<void(SharedResourceType&)>& manipulator)
{
    ((SharedResource<SharedResourceType>*)mISharedResource.get())->manipulate(manipulator);
}

template<typename ObjPtr, typename FuncPtr, class... Args>
void ethr::EventThread::callQueued(ObjPtr objPtr, FuncPtr funcPtr, Args... args)
{
    ((EventThread*)objPtr)->queueNewEvent(std::bind(funcPtr, objPtr, args...));
}

template<typename EthreadType, class... Args>
void ethr::EventThread::callInterthread(void(EthreadType::* func)(Args...), Args... args)
{
    for(const auto& ethreadPtr : EventThread::ethreads)
    {   if(typeid(*ethreadPtr) == typeid(EthreadType))
            callQueued((EthreadType*)ethreadPtr, func, args...);
    }
}

template<typename EthreadType, class... Args>
void ethr::EventThread::callInterthread(ThreadRef<EthreadType>& ref, void(EthreadType::* func)(Args...), Args... args)
{
    callQueued((EthreadType*)ref.mRef, func, args...);
}

#endif
