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
    SharedResource()
    {

    }
    void setSharedResource(T* sharedResource)
    {
        mSharedResource = sharedResource;
    }

    /*
    Template parameter lets compiler make classes for each ManipulatorType. Therefore manipulators doesn't have to reallocated every time, enhancing performance.
    */
    template<typename ManipulatorType>
    void manipulate(const ManipulatorType& manipulator)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        manipulator(mSharedResource);
    }
private:
    T mSharedResource;
    std::mutex mMutex;
};

}

#endif