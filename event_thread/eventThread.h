#ifndef EVENTTHREAD_H
#define EVENTTHREAD_H

#include <pthread.h>
#include <iostream>
#include <mutex>
#include <functional>
#include <queue>
#include <vector>

#define NSEC_PER_SEC 1000000000
#define events public

class EventThread
{
public:
    EventThread();
    ~EventThread();
    void start();
    void stop();
    void setLoopFreq(double freq);
    template<typename ObjPtr, typename FuncPtr, class... Args>
    static void callQueued(ObjPtr objPtr, FuncPtr funcPtr, Args... args);
    template<typename EThreadType, class... Args>
    static void callInterthread(void(EThreadType::* func)(Args...), Args... args);
protected:
    virtual void task()=0;
private:
    pthread_t mPthread;
    unsigned int mLoopInterval; // loop interval in nanoseconds
    std::mutex mMutexLoop, mMutexEvent;
    std::queue<std::function<void(void)>> mEventQueue;
    size_t mEventQueueSize;
    bool mIsLoopRunning;
    void handleQueuedEvents();
    void runLoop();
    void queueNewEvent(const std::function<void ()> &func);

    static void* threadEntryPoint(void* param);
    static std::vector<EventThread*> ethreads;
};

template<typename ObjPtr, typename FuncPtr, class... Args>
void EventThread::callQueued(ObjPtr objPtr, FuncPtr funcPtr, Args... args)
{
    ((EventThread*)objPtr)->queueNewEvent(std::bind(funcPtr, objPtr, args...));
}

template<typename EThreadType, class... Args>
void EventThread::callInterthread(void(EThreadType::* func)(Args...), Args... args)
{
    for(const auto ethreadPtr : EventThread::ethreads)
    {
        if(typeid(*ethreadPtr) == typeid(EThreadType))
        {
            //std::cout<<"found "<<typeid(*ethreadPtr).name()<<std::endl;
            callQueued((EThreadType*)ethreadPtr, func, args...);
        }
    }
}
#endif
