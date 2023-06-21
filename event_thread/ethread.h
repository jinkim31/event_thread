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

    explicit EThread(const std::string &name = "unnamed");

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

protected:
    virtual void task()
    {};      // virtual function that runs in the loop

    virtual void onStart()
    {};   // virtual function that runs once when the thread starts

    virtual void onTerminate()
    {};

private:
    std::thread mThread;
    bool mIsInNewThread;
    std::string mName;
    std::mutex mMutexLoop, mMutexEvent;
    std::deque<std::pair<EObject *, std::function<void(void)>>> mEventQueue;
    size_t mEventQueueSize;
    std::chrono::high_resolution_clock::duration mTaskPeriod;
    std::chrono::time_point<std::chrono::high_resolution_clock> mNextTaskTime;
    bool mIsLoopRunning;
    EventHandleScheme mEventHandleScheme;
    std::vector<EObject *> mChildEObjects;
    static std::map<std::thread::id, EThread *> ethreads;

    bool checkLoopRunningSafe();

    void queueNewEvent(EObject *eObjectPtr, const std::function<void()> &func);

    void runLoop();

    static void *threadEntryPoint(void *param);

    void notifyEObjectDestruction(EObject *eObjectPtr);

    void addChildEObject(EObject *eObjectPtr);

    void removeChildEObject(EObject *eObjectPtr);
    friend EObject;
};

class EObject
{
public:
    explicit EObject(EThread *ethreadPtr = nullptr);
    virtual ~EObject();

    template<typename ObjType, class... Args>
    void callQueued(void (ObjType::*funcPtr)(Args...), Args... args)
    {
        if (mParentThread == nullptr)
        {
            std::cerr
                    << "[EThread] EObject::callQueued() is called but no EThread is assigned to it."
                    << std::endl;
            return;
        }
        mParentThread->queueNewEvent(this, std::bind(funcPtr, (ObjType *) this, args...));
    }

    static void runQueued(EObject* eObjectPtr, const std::function<void(void)>& functor)
    {
        if (eObjectPtr->mParentThread == nullptr)
        {
            std::cerr
                    << "[EThread] EObject::callQueued() is called but no EThread is assigned to it."
                    << std::endl;
            return;
        }
        eObjectPtr->mParentThread->queueNewEvent(eObjectPtr, functor);
    }

    void moveToThread(EThread &ethread);

    void notifyEThreadDestruction(EThread *eThreadPtr);
private:
    EThread *mParentThread;
};

}
#endif
