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
// forward decl
class EObject;
template <class>
class EObjectRef;
class UntypedEObjectRef;
template<typename PromiseType, typename... ParamTypes>
class EPromise;

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
        explicit MainEThreadNotAssignedException(const std::string& what) : std::runtime_error(what){}
    };

    explicit EThread(const std::string &name = "unnamed");

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
    static EThread & mainThread();

protected:
    virtual void task()
    {};

    virtual void onStart()
    {};

    virtual void onTerminate()
    {};

private:
    std::thread mThread;
    bool mIsMain;
    std::string mName;
    std::mutex mMutexLoop, mMutexEventQueue, mMutexEventHandling, mMutexChildObjects;
    std::deque<std::pair<int, std::function<void(void)>>> mEventQueue;
    size_t mEventQueueSize;
    std::chrono::high_resolution_clock::duration mLoopPeriod;
    std::chrono::time_point<std::chrono::high_resolution_clock> mNextTaskTime;
    bool mIsLoopRunning;
    EventHandleScheme mEventHandleScheme;
    std::vector<int> mChildEObjectsIds;
    static EThread* mainEThreadPtr;

    bool checkLoopRunningSafe();

    void queueNewEvent(int eObjectId, const std::function<void()> &func);

    void runLoop();

    static void *threadEntryPoint(void *param);

    void addChildEObject(EObject *eObjectPtr);

    void removeChildEObject(EObject *eObjectPtr);

    friend EObject;
    template <class> friend class EObjectRef;
};

class EObject
{
public:
    class EObjectDestructedInThreadException : public std::runtime_error
    {
    public:
        explicit EObjectDestructedInThreadException(const std::string& what) : std::runtime_error(what){}
    };

    EObject();

    virtual ~EObject();

    template<typename RetType, typename ObjType, class... Args>
    void callQueued(RetType (ObjType::*funcPtr)(Args...), Args... args)
    {
        if (mThreadInAffinity == nullptr)
            throw std::runtime_error("EObject::callQueued() is called but no EThread is assigned to it.");
        mThreadInAffinity->queueNewEvent(mId, std::bind(funcPtr, (ObjType *) this, args...));
    }

    void runQueued(const std::function<void(void)>& functor)
    {
        mThreadInAffinity->queueNewEvent(mId, functor);
    }

    void moveToThread(EThread &ethread);

    void removeFromThread();

    template <class T>
    EObjectRef<T> ref()
    {
        return EObjectRef<T>(mId, dynamic_cast<T*>(this));
    }
protected:
    EThread & threadInAffinity();
private:
    int mId;
    EThread *mThreadInAffinity;
    static int idCount;
    static std::map<int, EObject*> activeEObjectIds;
    static std::shared_mutex mutexActiveEObjectIds;
friend EThread;
friend UntypedEObjectRef;
template <class> friend class EObjectRef;
};

class UntypedEObjectRef
{
public:
    UntypedEObjectRef()
    {
        mInitialized = false;
    }
    bool isInitialized() const
    {
        return mInitialized;
    }
    bool runQueued(const std::function<void(void)>& functor) const
    {
        std::shared_lock<std::shared_mutex> lock(EObject::mutexActiveEObjectIds);
        auto eObjectPtrIter = EObject::activeEObjectIds.find(mEObjectId);
        if (eObjectPtrIter == EObject::activeEObjectIds.end())
            return false;
        eObjectPtrIter->second->runQueued(functor);
        return true;
    }
protected:
    int mEObjectId;
    EObject* mUntypedEObjectUnsafePtr;
    bool mInitialized;
};

template <class EObjectType>
class EObjectRef : public UntypedEObjectRef
{
public:
    EObjectRef()=default;
    template<typename RetType, class... Args>
    bool callQueued(RetType (EObjectType::*funcPtr)(Args...), Args... args)
    {
        std::shared_lock<std::shared_mutex> lock(EObject::mutexActiveEObjectIds);
        auto eObjectPtrIter = EObject::activeEObjectIds.find(mEObjectId);
        if (eObjectPtrIter == EObject::activeEObjectIds.end())
            return false;
        eObjectPtrIter->second->callQueued(funcPtr, args...);
        return true;
    }

private:
    // private constructor so user can't create ref.
    EObjectRef(int eObjectId, EObjectType* eObjectPtr)
    {
        mEObjectId = eObjectId;
        mUntypedEObjectUnsafePtr = eObjectPtr;
        mEObjectUnsafePtr = (EObjectType*)eObjectPtr;
        mInitialized = true;
    }

    EObjectType* mEObjectUnsafePtr;
    friend EObject;
    template<typename, typename...> friend class EPromise;
};

}
#endif
