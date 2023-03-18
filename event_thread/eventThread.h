#ifndef EVENT_THREAD_H
#define EVENT_THREAD_H

/*
 * uncomment the following line to use pthread instead of std::thread
 */
//#define ETHREAD_USE_PTHREAD

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <queue>
#include <vector>
#include <chrono>
#include <memory>

#ifdef ETHREAD_USE_PTHREAD
#include <pthread.h>
#else
#include <thread>
#endif

#define events public

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
        USER_CONTROLLED,
    };

    EventThread(const std::string& name="");

    ~EventThread();

    /**
     * @brief Start thread.
     * 
     */
    void start(bool isMain = false);

    /**
     * @brief Stop thread. This should be called before EventThread destruction.
     * 
     */
    void stop();

    void setName(const std::string& name);

    /**
     * @brief Set the loop period in nanoseconds.
     * 
     * @param period
     */
    void setLoopPeriod(std::chrono::duration<long long int, std::nano> period);

    /**
     * @brief Set the loop frequency in Hz.
     * 
     * @param freq 
     */
    void setLoopFreq(const unsigned int freq);

    /**
     * @brief Set the event handling scheme. 
     * 
     * @param scheme 
     *  AFTER_TASK: handles event after task() execution
     *  BEFORE_TASK: handles event before task() execution
     *  USER_CONTROLLED: event handling is triggered when user explicitly calls handleQueuedEvents()
     */
    void setEventHandleScheme(EventHandleScheme scheme);

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
    static void callInterThread(void(EthreadType::* func)(Args...), Args... args);

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
    static void callInterThread(ThreadRef<EthreadType>& ref, void(EthreadType::* func)(Args...), Args... args);

    template<typename EthreadType>
    static bool findThread(ThreadRef<EthreadType>& ref, const std::string& name="");

protected:
    virtual void task()=0;      // pure virtual function that runs in the loop

    virtual void onStart(){};   // pure virtual function that runs once when the thread starts

    virtual void onTerminate(){};

    void handleQueuedEvents();

    template<typename SharedResourceType>
    void makeSharedResource();

private:
#ifdef ETHREAD_USE_PTHREAD
    pthread_t mThread;
#else
    std::thread mThread;
#endif
    // main thread refers the thread that is started blocking the application's thread
    bool mIsMainThread;
    std::string mName;
    std::mutex mMutexLoop, mMutexEvent;
    std::queue<std::function<void(void)>> mEventQueue;
    size_t mEventQueueSize;
    std::chrono::high_resolution_clock::duration  mTaskPeriod;
    std::chrono::time_point<std::chrono::high_resolution_clock> mNextTaskTime;
    bool mIsLoopRunning;
    EventHandleScheme mEventHandleScheme;

    bool checkLoopRunningSafe();

    void queueNewEvent(const std::function<void ()> &func);

    void runLoop();

    static void* threadEntryPoint(void* param);

    static std::vector<EventThread*> ethreads;
};

template <typename EthreadType>
class ThreadRef
{
public:
    ThreadRef()
    {
        mHasRef=false;
    }

    bool hasRef()
    {
        return mHasRef;
    }
private:
    EventThread* mRef;
    bool mHasRef;
friend ethr::EventThread;
};

template<typename T>
class SafeSharedPtr
{
public:
    using ReadOnlyPtr = const std::shared_ptr<const T>;
    using ReadWritePtr = const std::shared_ptr<T>;
    /* constructors */
    SafeSharedPtr();
    SafeSharedPtr(std::shared_ptr<T> var);
    /* copy constructor */
    SafeSharedPtr(const SafeSharedPtr& safeType);

    // minimize code that goes into the lambda since it will be ran in a critical section
    template<typename Manip>    // functor template avoids reallocations. Manip has to be the type: void(const std::shared_ptr<const T>)
    void readOnly(Manip manip)
    {
        std::shared_lock lock(*mMutexPtr);
        const std::shared_ptr<const T>& constVar = mVar;
        manip(constVar);
    }

    // minimize code that goes into the lambda since it will be ran in a critical section
    template<typename Manip>    // functor template avoids reallocations. Manip has to be the type: void(const std::shared_ptr<T>)
    void readWrite(Manip manip)
    {
        std::unique_lock lock(*mMutexPtr);
        manip(mVar);
    }
private:
    std::shared_ptr<T> mVar;
    std::shared_ptr<std::shared_mutex> mMutexPtr;
};

}   // namespace ethr

template<typename EthreadType>
bool ethr::EventThread::findThread(ThreadRef<EthreadType>& ref, const std::string& name)
{
    for(const auto& ethreadPtr : EventThread::ethreads)
    {   if(typeid(*ethreadPtr) == typeid(EthreadType) && ethreadPtr->mName == name)
        {
                ref.mRef = ethreadPtr;
                ref.mHasRef = true;
                return true;
        }
    }
    return false;
}

template<typename ObjPtr, typename FuncPtr, class... Args>
void ethr::EventThread::callQueued(ObjPtr objPtr, FuncPtr funcPtr, Args... args)
{
    ((EventThread*)objPtr)->queueNewEvent(std::bind(funcPtr, objPtr, args...));
}

template<typename EthreadType, class... Args>
void ethr::EventThread::callInterThread(void(EthreadType::* func)(Args...), Args... args)
{
    for(const auto& ethreadPtr : EventThread::ethreads)
    {
        if(typeid(*ethreadPtr) == typeid(EthreadType) || dynamic_cast<EthreadType*>(ethreadPtr))
        {
            //std::cout<<"match:"<<ethreadPtr->mName<<std::endl;
            callQueued((EthreadType*) ethreadPtr, func, args...);
        }
    }
}

template<typename EthreadType, class... Args>
void ethr::EventThread::callInterThread(ThreadRef<EthreadType>& ref, void(EthreadType::* func)(Args...), Args... args)
{
    callQueued((EthreadType*)ref.mRef, func, args...);
}

template<typename T>
ethr::SafeSharedPtr<T>::SafeSharedPtr()
{
    mMutexPtr = std::make_shared<std::shared_mutex>();
}

template<typename T>
ethr::SafeSharedPtr<T>::SafeSharedPtr(std::shared_ptr<T> var)
{
    mMutexPtr = std::make_shared<std::shared_mutex>();
    mVar = var;
}

template<typename T>
ethr::SafeSharedPtr<T>::SafeSharedPtr(const SafeSharedPtr &safePtrType)
{
    mVar = safePtrType.mVar;
    mMutexPtr = safePtrType.mMutexPtr;
}
#endif
