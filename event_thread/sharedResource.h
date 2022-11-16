#ifndef SHARED_RESOURCE_H
#define SHARED_RESOURCE_H

#include <iostream>
#include <mutex>
#include <functional>

namespace ethr
{

/* empty class for generic polymorphism */
class ISharedResource
{
public:
    virtual ~ISharedResource(){};
};

template<typename T>
class SharedResource : public ISharedResource
{
public:
    SharedResource();
    void setSharedResource(T* sharedResource);
    void manipulate(const std::function<void(T&)>& manipulator);
private:
    T mSharedResource;
    std::mutex mMutex;
};

}

template<typename T>
ethr::SharedResource<T>::SharedResource()
{

}

template<typename T>
void ethr::SharedResource<T>::manipulate(const std::function<void(T&)>& manipulator)
{
    std::unique_lock<std::mutex> lock(mMutex);
    manipulator(mSharedResource);
}

template<typename T>
void ethr::SharedResource<T>::setSharedResource(T* sharedResource)
{
    mSharedResource = sharedResource;
}

#endif