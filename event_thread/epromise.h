#ifndef EVENT_THREAD_EPROMISE_H
#define EVENT_THREAD_EPROMISE_H

#include "ethread.h"

namespace ethr
{

class ExceptionNotCaughtException : public std::runtime_error
{
public:
    ExceptionNotCaughtException(const std::string& what) : std::runtime_error(what){}
};

class EDeletable
{
public:
    virtual ~EDeletable() = default;
};

template<typename PromiseType, typename... ParamTypes>
class EPromise : public EDeletable
{
public:
    EPromise(EObject *eObjectPtr, const std::function<PromiseType(ParamTypes...)> functor)
    {
        mTargetEObjectId = eObjectPtr->id();
        mExecuteFunctor = functor;
        mThenPromisePtr = nullptr;
        mCatchEObjectId = -1;
        mInitialized = true;
    }

    template<typename EObjectType>
    EPromise(EObjectType *eObjectPtr, PromiseType(EObjectType::*funcPtr)(ParamTypes...))
    : EPromise(eObjectPtr, [=](ParamTypes... params){return ((*eObjectPtr).*funcPtr)(params...);}){}

    ~EPromise()
    {

    }

    void selfDestructChain()
    {
        // TODO: destruct promises after exception catch
        delete this;
    }
    void execute(ParamTypes... params)
    {
        if (!mInitialized)
            return;

        EObject::runQueued(mTargetEObjectId, [=]
        {
            try
            {
                if (mThenPromisePtr)
                    mExecuteThenFunctor(mExecuteFunctor(params...));
                else
                    mExecuteFunctor(params...);
            }
            catch (const std::exception& e)
            {
                if (mCatchEObjectId == -1)
                    throw ExceptionNotCaughtException(
                            "[EThread] Detected uncaught exception: \"" + std::string(e.what())
                            + "\". Use EPromise::cat() to catch the exception.");

                std::exception_ptr eptr = std::current_exception();
                EObject::runQueued(mCatchEObjectId, [=]{
                    mCatchFunctor(eptr);
                });
            }
            delete this;
        });
    }

    template<typename EObjectType>
    EPromise<PromiseType, ParamTypes...>* cat(
            EObjectType *eObjectPtr,
            void(EObjectType::*funcPtr)(std::exception_ptr))
    {
        mCatchEObjectId = eObjectPtr;
        mCatchFunctor = std::bind(funcPtr, eObjectPtr, std::placeholders::_1);
        return this;
    }

    EPromise<PromiseType, ParamTypes...>* cat(
            EObject *eObjectPtr,
            const std::function<void(std::exception_ptr)> functor)
    {
        mCatchEObjectId = eObjectPtr->id();
        mCatchFunctor = functor;
        return this;
    }

    template<typename ThenPromiseType, typename EObjectType>
    EPromise<ThenPromiseType, PromiseType>* then(
            EObjectType *eObjectPtr,
            ThenPromiseType(EObjectType::*funcPtr)(PromiseType))
    {
        auto thenPromise = new EPromise<ThenPromiseType, PromiseType>(eObjectPtr, funcPtr);
        mExecuteThenFunctor = [=](PromiseType output)
        { thenPromise->execute(output); };
        mThenPromisePtr = thenPromise;
        return thenPromise;
    }

    template<typename ThenPromiseType>
    EPromise<ThenPromiseType, PromiseType>* then(
            EObject *eObjectPtr,
            const std::function<ThenPromiseType(PromiseType)>& functor)
    {
        auto thenPromise = new EPromise<ThenPromiseType, PromiseType>(eObjectPtr, functor);
        mExecuteThenFunctor = [=](PromiseType output)
        { thenPromise->execute(output); };
        mThenPromisePtr = thenPromise;
        return thenPromise;
    }

private:
    bool mInitialized;
    int mTargetEObjectId;
    std::function<void(PromiseType)> mExecuteThenFunctor;
    std::function<PromiseType(ParamTypes...)> mExecuteFunctor;
    int mCatchEObjectId;
    std::function<void(std::exception_ptr)> mCatchFunctor;
    EDeletable *mThenPromisePtr;
};
}

#endif
