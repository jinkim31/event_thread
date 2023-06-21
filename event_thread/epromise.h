#ifndef EVENT_THREAD_EPROMISE_H
#define EVENT_THREAD_EPROMISE_H

#include "ethread.h"

namespace ethr
{

class EDeletable
{
public:
    virtual ~EDeletable() = default;
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
        if (mThenPromisePtr)
            delete mThenPromisePtr;
    }

    void execute(ParamTypes... params)
    {
        if (!mInitialized)
            return;

        EObject::runQueued(mTargetEObjectPtr, [=]
        {
            try
            {
                if (mThenPromisePtr) mExecuteThenFunctor(mExecuteFunctor(params...));
                else mExecuteFunctor(params...);
            }
            catch (...)
            {
                if (!mCatchEObjectPtr)
                    return;
                EObject::runQueued(mCatchEObjectPtr, [=]
                { mCatchFunctor(std::current_exception()); });
            }
        });
    }

    template<typename EObjectType>
    EPromise<PromiseType, ParamTypes...> &cat(
            EObjectType *eObjectPtr,
            void(EObjectType::*funcPtr)(std::exception_ptr))
    {
        mCatchEObjectPtr = eObjectPtr;
        mCatchFunctor = std::bind(funcPtr, eObjectPtr, std::placeholders::_1);
        return *this;
    }

    template<typename ThenPromiseType, typename EObjectType>
    EPromise<PromiseType, ThenPromiseType> &then(
            EObjectType *eObjectPtr,
            ThenPromiseType(EObjectType::*funcPtr)(PromiseType))
    {
        auto thenPromise = new EPromise<PromiseType, ThenPromiseType>(eObjectPtr, funcPtr);
        mExecuteThenFunctor = [=](PromiseType output)
        { thenPromise->execute(output); };
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
    EDeletable *mThenPromisePtr;
};
}

#endif
