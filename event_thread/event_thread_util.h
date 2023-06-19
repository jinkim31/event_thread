#ifndef EVENT_THREAD_UTIL_H
#define EVENT_THREAD_UTIL_H

#include "event_thread.h"

namespace ethr
{

class ELoopObserver : public EObject
{
public:
    ELoopObserver();
protected:
    void start();
    void stop();
    virtual void loopObserverCallback() = 0;
    void chainCallback();
    bool mIsObserving;
};

class ETimer : public ELoopObserver
{
public:
    void start();
    void stop();
    void addTask(
            const int &id,
            const std::function<void(void)> &callback,
            const std::chrono::high_resolution_clock::duration &period,
            const int &timeToLive = -1);
    void removeTask(const int &id);
private:
    struct Task
    {
        const std::function<void(void)> callback;
        const std::chrono::high_resolution_clock::duration period;
        std::chrono::high_resolution_clock::time_point nextTaskTime;
        int timeToLive; // -1: continuous
    };
    std::map<int, Task> mTasks; // map of {id : task}
    void loopObserverCallback() override;
};

template<typename T>
class SafeSharedPtr
{
public:
    using ReadOnlyPtr = const std::shared_ptr<const T>;
    using ReadWritePtr = const std::shared_ptr<T>;
    // constructor
    SafeSharedPtr();
    explicit SafeSharedPtr(std::shared_ptr<T> var);
    // copy constructor
    SafeSharedPtr(const SafeSharedPtr &safeSharedPtr);

    // minimize code that goes into the lambda since it will be ran in a critical section
    template<typename Manip>
    // functor template avoids reallocation. Manip has to be the type: void(ReadOnlyPtr)
    void readOnly(Manip manip)
    {
        std::shared_lock lock(*mMutexPtr);
        const std::shared_ptr<const T> &constVar = mVar;
        manip(constVar);
    }

    // minimize code that goes into the lambda since it will be ran in a critical section
    template<typename Manip>
    // functor template avoids reallocation. Manip has to be the type: void(ReadWritePtr)
    void readWrite(Manip manip)
    {
        std::unique_lock lock(*mMutexPtr);
        manip(mVar);
    }

private:
    std::shared_ptr<T> mVar;
    std::shared_ptr<std::shared_mutex> mMutexPtr;
};


class EDeletable
{
public:
    virtual ~EDeletable()= default;
};

template<typename PromiseType, typename... ParamTypes>
class EPromise : public EDeletable
{
public:
    EPromise()
    {
        mInitialized = false;
    }

    template<typename EObjectType>
    EPromise(EObjectType *eObjectPtr, PromiseType(EObjectType::*funcPtr)(ParamTypes...))
    {
        mTargetEObjectPtr = eObjectPtr;
        //mExecuteFunctor = [=](ParamTypes... params){ return (*eObjectPtr.*funcPtr)(params...); };
        mExecuteFunctor = std::bind(funcPtr, eObjectPtr, std::placeholders::_1);
        mThenPromisePtr = nullptr;
        mCatchEObjectPtr = nullptr;
        mInitialized = true;
    }

    ~EPromise()
    {
        if(mThenPromisePtr != nullptr)
            delete mThenPromisePtr;
    }

    void execute(ParamTypes... params)
    {
        if(!mInitialized)
            return;

        EObject::runQueued(mTargetEObjectPtr, [=]
        {
            PromiseType ret;
            try{
                ret = mExecuteFunctor(params...);
                if(mThenPromisePtr != nullptr) mExecuteThenFunctor(ret);
            }
            catch(...) {
                if(mCatchEObjectPtr != nullptr)
                {
                    EObject::runQueued(mCatchEObjectPtr, [=]
                    { mCatchFunctor(std::current_exception()); });
                }
            }
        });
    }

    template<typename EObjectType>
    EPromise<PromiseType, ParamTypes...>& cat(
            EObjectType *eObjectPtr,
            void(EObjectType::*funcPtr)(std::exception_ptr))
    {
        mCatchEObjectPtr = eObjectPtr;
        mCatchFunctor = std::bind(funcPtr, eObjectPtr, std::placeholders::_1);
        return *this;
    }

    template<typename ThenPromiseType, typename EObjectType>
    EPromise<PromiseType, ThenPromiseType>& then(
            EObjectType *eObjectPtr,
            ThenPromiseType(EObjectType::*funcPtr)(PromiseType))
    {
        auto thenPromise = new EPromise<PromiseType, ThenPromiseType>(eObjectPtr, funcPtr);
        mExecuteThenFunctor = [=](PromiseType output){ thenPromise->execute(output); };
        mThenPromisePtr = thenPromise;
        return (*thenPromise);
    }

private:
    bool mInitialized;
    EObject *mTargetEObjectPtr;
    std::function<void(PromiseType)> mExecuteThenFunctor;
    std::function<PromiseType(ParamTypes...)> mExecuteFunctor;
    EObject *mCatchEObjectPtr;
    std::function<void(std::exception_ptr)> mCatchFunctor;
    EDeletable* mThenPromisePtr;
};

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

}
#endif
