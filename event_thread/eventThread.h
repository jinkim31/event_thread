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
#include <map>

#ifdef ETHREAD_USE_PTHREAD
#include <pthread.h>
#else
#include <thread>
#endif

namespace ethr
{
class EThreadObject;

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
     * @brief call functions in a thread-safe manner
     * @param funcPtr void function pointer
     * @param args arguments
     */
    template<typename ObjType, class... Args>
    void callQueued(void (ObjType::*funcPtr)(Args...), Args... args)
    {
        queueNewEvent(std::bind(funcPtr, (ObjType*)this, args...));
    }

    void handleQueuedEvents();

protected:
    virtual void task(){};      // virtual function that runs in the loop

    virtual void onStart(){};   // virtual function that runs once when the thread starts

    virtual void onTerminate(){};

private:
#ifdef ETHREAD_USE_PTHREAD
    pthread_t mThread;
#else
    std::thread mThread;
#endif
    // main thread refers the thread that is started blocking the application's thread
    bool isInNewThread;
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

class EThreadObject
{
public:
    explicit EThreadObject(EThread* ethreadPtr = nullptr);

    template<typename ObjType, class... Args>
    void callQueued(void (ObjType::*funcPtr)(Args...), Args... args)
    {
        if(mParentThread == nullptr)
        {
            std::cerr
            <<"[EThread] EThreadObject::callQueued() is called but no EThread is assigned to it."
            <<std::endl;
            return;
        }
        mParentThread->queueNewEvent(std::bind(funcPtr, (ObjType*)this, args...));
    }

    void moveToThread(EThread& ethread);
private:
    EThread* mParentThread;
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
