#ifndef EVENT_THREAD_H
#define EVENT_THREAD_H

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <queue>
#include <vector>
#include <chrono>
#include <memory>
#include <map>
#include <thread>

namespace ethr
{
class EObject;

class EThread
{
public:
    enum class EventHandleScheme
    {
        AFTER_TASK,
        BEFORE_TASK,
        USER_CONTROLLED,
    };

    class MainEThreadNotAssignedException : public std::runtime_error
    {
    public:
        MainEThreadNotAssignedException(const std::string& what) : std::runtime_error(what){}
    };

    EThread(const std::string &name = "unnamed");

    ~EThread();

    /**
     * @brief Start thread.
     *
     */
    void start();

    /**
     * @brief Stop thread. This should be called before EThread destruction.
     *
     */
    void stop();

    void setName(const std::string &name);

    /**
     * @brief Set the loop period.
     *
     * @param period
     */
    void setLoopPeriod(std::chrono::duration<long long int, std::nano> period);

    /**
     * @brief Set the loop frequency in Hz.
     *
     * @param freq
     */
    void setLoopFreq(const unsigned int &freq);

    /**
     * @brief Set the event handling scheme.
     *
     * @param scheme
     *  AFTER_TASK: handles event after task() execution
     *  BEFORE_TASK: handles event before task() execution
     *  USER_CONTROLLED: event handling is triggered when user explicitly calls handleQueuedEvents()
     */
    void setEventHandleScheme(EventHandleScheme scheme);

    void handleQueuedEvents();

    static void provideMainThread(EThread& ethread);
    static void stopMainThread();

protected:
    virtual void task()
    {};

    virtual void onStart()
    {};

    virtual void onTerminate()
    {};

    EThread & mainThread();

private:
    std::thread mThread;
    bool mIsMain;
    std::string mName;
    std::mutex mMutexLoop, mMutexEvent, mMutexEObjects;
    std::deque<std::pair<EObject *, std::function<void(void)>>> mEventQueue;
    size_t mEventQueueSize;
    std::chrono::high_resolution_clock::duration mTaskPeriod;
    std::chrono::time_point<std::chrono::high_resolution_clock> mNextTaskTime;
    bool mIsLoopRunning;
    EventHandleScheme mEventHandleScheme;
    std::vector<EObject *> mChildEObjects;
    static EThread* mainEThreadPtr;

    bool checkLoopRunningSafe();

    void queueNewEvent(EObject *eObjectPtr, const std::function<void()> &func);

    void runLoop();

    static void *threadEntryPoint(void *param);

    void addChildEObject(EObject *eObjectPtr);

    void removeChildEObject(EObject *eObjectPtr);

    friend EObject;
};

class EObject
{
public:
    class EObjectDestructedInThreadException : public std::runtime_error
    {
    public:
        EObjectDestructedInThreadException(const std::string& what) : std::runtime_error(what){}
    };

    EObject();
    virtual ~EObject();

    template<typename RetType, typename ObjType, class... Args>
    void callQueued(RetType (ObjType::*funcPtr)(Args...), Args... args)
    {
        if (mThreadInAffinity == nullptr)
            throw std::runtime_error("EObject::callQueued() is called but no EThread is assigned to it.");
        mThreadInAffinity->queueNewEvent(this, std::bind(funcPtr, (ObjType *) this, args...));
    }

    static void runQueued(EObject* eObjectPtr, const std::function<void(void)>& functor)
    {
        if (eObjectPtr->mThreadInAffinity == nullptr)
            throw std::runtime_error("EObject::callQueued() is called but no EThread is assigned to it.");
        eObjectPtr->mThreadInAffinity->queueNewEvent(eObjectPtr, functor);
    }

    void addToThread(EThread &ethread);
    void removeFromThread();

protected:
    EThread & threadInAffinity();
private:
    EThread *mThreadInAffinity;
    std::shared_mutex mMutex;
    void notifyEThreadDestruction(EThread *eThreadPtr);
friend EThread;
};

}
#endif
