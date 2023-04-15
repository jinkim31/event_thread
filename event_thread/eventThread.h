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
#include <map>

#endif

namespace ethr
{
    class EThread;

class EThreadObject
{
public:
    explicit EThreadObject(EThread* ethreadPtr = nullptr);

    template<typename ObjPtr, typename FuncPtr, class... Args>
    void callQueued(ObjPtr objPtr, FuncPtr funcPtr, Args... args);

    void moveToThread(EThread* ethread);
private:
    EThread* mParentThread;
};

class EThread
{
public:
    enum class EventHandleScheme
    {
        AFTER_TASK,
        BEFORE_TASK,
        USER_CONTROLLED,
    };

    explicit EThread(const std::string& name="unnamed");

    ~EThread();

    /**
     * @brief Start thread.
     * 
     */
    void start(bool makeNewThread = true);

    /**
     * @brief Stop thread. This should be called before EThread destruction.
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
    void setLoopFreq(const unsigned int& freq);

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
     * @param objPtr ObjPtr EThread object pointer to add event to
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
     * @param func EThread event function pointer
     * @param args EThread event function arguments
     */
    template<typename EthreadType, class... Args>
    static void callInterThread(void(EthreadType::* func)(Args...), Args... args);
protected:
    virtual void task(){};      // virtual function that runs in the loop

    virtual void onStart(){};   // virtual function that runs once when the thread starts

    virtual void onTerminate(){};

    void handleQueuedEvents();

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

    static std::map<std::thread::id, EThread*> ethreads;
friend EThreadObject;
};

template<typename T>
class SafeSharedPtr
{
public:
    using ReadOnlyPtr = const std::shared_ptr<const T>;
    using ReadWritePtr = const std::shared_ptr<T>;
    /* constructors */
    SafeSharedPtr();
    explicit SafeSharedPtr(std::shared_ptr<T> var);
    /* copy constructor */
    SafeSharedPtr(const SafeSharedPtr& safeSharedPtr);

    // minimize code that goes into the lambda since it will be ran in a critical section
    template<typename Manip>    // functor template avoids relocations. Manip has to be the type: void(const std::shared_ptr<const T>)
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

template<typename ObjPtr, typename FuncPtr, class... Args>
void ethr::EThreadObject::callQueued(ObjPtr objPtr, FuncPtr funcPtr, Args... args)
{
    mParentThread->callQueued(objPtr, funcPtr);
}

template<typename ObjPtr, typename FuncPtr, class... Args>
void ethr::EThread::callQueued(ObjPtr objPtr, FuncPtr funcPtr, Args... args)
{
    ((EThread*)objPtr)->queueNewEvent(std::bind(funcPtr, objPtr, args...));
}

template<typename EthreadType, class... Args>
void ethr::EThread::callInterThread(void(EthreadType::* func)(Args...), Args... args)
{
    for (const auto & [threadId, ethreadPtr] : EThread::ethreads)
    {
        if(typeid(*ethreadPtr) == typeid(EthreadType) || dynamic_cast<EthreadType*>(ethreadPtr))
        {
            //std::cout<<"match:"<<ethreadPtr->mName<<std::endl;
            callQueued((EthreadType*) ethreadPtr, func, args...);
        }
    }
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
ethr::SafeSharedPtr<T>::SafeSharedPtr(const SafeSharedPtr &safeSharedPtr)
{
    mVar = safeSharedPtr.mVar;
    mMutexPtr = safeSharedPtr.mMutexPtr;
}
#endif
