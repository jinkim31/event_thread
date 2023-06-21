#ifndef EVENT_THREAD_UTIL_H
#define EVENT_THREAD_UTIL_H

#include "ethread.h"

namespace ethr
{

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
