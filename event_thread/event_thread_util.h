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


template<typename InputType, typename OutputType>
class EPromise
{
public:
    template<typename EObjectType>
    EPromise(EObjectType *eObjectPtr, OutputType(EObjectType::*funcPtr)(InputType))
    {
        mTargetEObjectPtr = eObjectPtr;
        mExecuteFunctor = [=](InputType input){ return (*eObjectPtr.*funcPtr)(input); };
        mThenAssigned = false;
    }

    void execute(InputType value)
    {
        EObject::runQueued(mTargetEObjectPtr, [=]
        {
            auto ret = mExecuteFunctor(value);
            if(mThenAssigned) mExecuteThenFunctor(ret);
        });
    }

    template<typename ThenOutputType, typename EObjectType>
    EPromise<OutputType, ThenOutputType>& then(
            EObjectType *eObjectPtr,
            ThenOutputType(EObjectType::*funcPtr)(OutputType))
    {
        auto thenPromise = new EPromise<OutputType, ThenOutputType>(eObjectPtr, funcPtr);
        mExecuteThenFunctor = [=](OutputType output){ thenPromise->execute(output); };
        mThenAssigned = true;
        return (*thenPromise);
    }
private:
    EObject *mTargetEObjectPtr;
    std::function<void(OutputType)> mExecuteThenFunctor;
    std::function<OutputType(InputType)> mExecuteFunctor;
    bool mThenAssigned;
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
