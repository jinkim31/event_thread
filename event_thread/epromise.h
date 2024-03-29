#ifndef EVENT_THREAD_EPROMISE_H
#define EVENT_THREAD_EPROMISE_H

#include "ethread.h"

namespace ethr
{

class ExceptionNotCaughtException : public std::runtime_error
{
public:
    explicit ExceptionNotCaughtException(const std::string& what) : std::runtime_error(what){}
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
    EPromise(UntypedEObjectRef eObjectRef, const std::function<PromiseType(ParamTypes...)> functor)
    {
        if(!eObjectRef.isInitialized())
            throw std::runtime_error("[EThread] EPromise is created using empty EObject reference.");
        mTargetEObjectRef = eObjectRef;
        mExecuteFunctor = functor;
        mThenPromisePtr = nullptr;
        mInitialized = true;
    }

    template<typename EObjectType>
    EPromise(EObjectRef<EObjectType> eObjectRef, PromiseType(EObjectType::*funcPtr)(ParamTypes...))
    : EPromise(eObjectRef, [=](ParamTypes... params){return ((*(eObjectRef.eObjectUnsafePtr())).*funcPtr)(params...);}){}

    void selfDestructChain()
    {
        // TODO: destruct promises after exception catch
        delete this;
    }
    void execute(ParamTypes... params)
    {
        if (!mInitialized)
            return;

        mTargetEObjectRef.runQueued([&, params...]
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
                if (!mCatchEObjectRef.isInitialized())
                    throw ExceptionNotCaughtException(
                            "[EThread] Detected uncaught exception: \"" + std::string(e.what())
                            + "\". Use EPromise::cat() to catch the exception.");

                std::exception_ptr eptr = std::current_exception();
                mCatchEObjectRef.runQueued([&]{
                    mCatchFunctor(eptr);
                });
            }

            delete this;
        });
    }

    template<typename EObjectType>
    EPromise<PromiseType, ParamTypes...>* cat(
            EObjectRef<EObjectType> eObjectRef,
            void(EObjectType::*funcPtr)(std::exception_ptr))
    {
        mCatchEObjectRef = eObjectRef;
        mCatchFunctor = std::bind(funcPtr, eObjectRef.mUntypedEObjectUnsafePtr, std::placeholders::_1);
        return this;
    }

    EPromise<PromiseType, ParamTypes...>* cat(
            UntypedEObjectRef eObjectRef,
            const std::function<void(std::exception_ptr)>& functor)
    {
        mCatchEObjectRef = eObjectRef;
        mCatchFunctor = functor;
        return this;
    }

    template<typename ThenPromiseType, typename EObjectType>
    EPromise<ThenPromiseType, PromiseType>* then(
            EObjectRef<EObjectType> eObjectRef,
            ThenPromiseType(EObjectType::*funcPtr)(PromiseType))
    {
        auto thenPromise = new EPromise<ThenPromiseType, PromiseType>(eObjectRef, funcPtr);
        mExecuteThenFunctor = [=](PromiseType output){ thenPromise->execute(output); };
        mThenPromisePtr = thenPromise;
        return thenPromise;
    }

    template<typename ThenPromiseType>
    EPromise<ThenPromiseType, PromiseType>* then(
            UntypedEObjectRef eObjectRef,
            const std::function<ThenPromiseType(PromiseType)>& functor)
    {
        auto thenPromise = new EPromise<ThenPromiseType, PromiseType>(eObjectRef, functor);
        mExecuteThenFunctor = [=](PromiseType output){ thenPromise->execute(output); };
        mThenPromisePtr = thenPromise;
        return thenPromise;
    }

private:
    bool mInitialized;
    UntypedEObjectRef mTargetEObjectRef;
    std::function<void(PromiseType)> mExecuteThenFunctor;
    std::function<PromiseType(ParamTypes...)> mExecuteFunctor;
    UntypedEObjectRef mCatchEObjectRef;
    std::function<void(std::exception_ptr)> mCatchFunctor;
    EDeletable *mThenPromisePtr;
};

template<typename PromiseType, typename... ParamTypes>
class EPromiseMove : public EDeletable
{
public:
    EPromiseMove(UntypedEObjectRef eObjectRef, const std::function<PromiseType(ParamTypes&&...)> functor)
    {
        if(!eObjectRef.isInitialized())
            throw std::runtime_error("[EThread] EPromiseMove is created using empty EObject reference.");
        mTargetEObjectRef = eObjectRef;
        mExecuteFunctor = functor;
        mThenPromisePtr = nullptr;
        mInitialized = true;
    }

    template<typename EObjectType>
    EPromiseMove(EObjectRef<EObjectType> eObjectRef, PromiseType(EObjectType::*funcPtr)(ParamTypes&&...))
            : EPromiseMove(eObjectRef, [=](ParamTypes&&... params){
                return ((*(eObjectRef.eObjectUnsafePtr())).*funcPtr)(std::move(params)...);
            }){}

    void selfDestructChain()
    {
        // TODO: destruct promises after exception catch
        delete this;
    }
    void execute(ParamTypes&&... params)
    {
        if (!mInitialized)
            return;

        mTargetEObjectRef.runQueued([&, ... params = std::move(params)]() mutable
        {
            try
            {
                if (mThenPromisePtr)
                    mExecuteThenFunctor(std::move(mExecuteFunctor(std::move(params)...)));
                else
                    mExecuteFunctor(std::move(params)...);
            }
            catch (const std::exception& e)
            {
                if (!mCatchEObjectRef.isInitialized())
                    throw ExceptionNotCaughtException(
                            "[EThread] Detected uncaught exception: \"" + std::string(e.what())
                            + "\". Use EPromiseMove::cat() to catch the exception.");

                std::exception_ptr eptr = std::current_exception();
                mCatchEObjectRef.runQueued([&]{
                    mCatchFunctor(eptr);
                });
            }

            delete this;
        });
    }

    template<typename EObjectType>
    EPromiseMove<PromiseType, ParamTypes...>* cat(
            EObjectRef<EObjectType> eObjectRef,
            void(EObjectType::*funcPtr)(std::exception_ptr))
    {
        mCatchEObjectRef = eObjectRef;
        mCatchFunctor = std::bind(funcPtr, eObjectRef.mUntypedEObjectUnsafePtr, std::placeholders::_1);
        return this;
    }

    EPromiseMove<PromiseType, ParamTypes...>* cat(
            UntypedEObjectRef eObjectRef,
            const std::function<void(std::exception_ptr)>& functor)
    {
        mCatchEObjectRef = eObjectRef;
        mCatchFunctor = functor;
        return this;
    }

    template<typename ThenPromiseType, typename EObjectType>
    EPromiseMove<ThenPromiseType, PromiseType>* then(
            EObjectRef<EObjectType> eObjectRef,
            ThenPromiseType(EObjectType::*funcPtr)(PromiseType&&))
    {
        auto thenPromise = new EPromiseMove<ThenPromiseType, PromiseType>(eObjectRef, funcPtr);
        mExecuteThenFunctor = [=](PromiseType&& output){ thenPromise->execute(std::move(output)); };
        mThenPromisePtr = thenPromise;
        return thenPromise;
    }

    template<typename ThenPromiseType>
    EPromiseMove<ThenPromiseType, PromiseType>* then(
            UntypedEObjectRef eObjectRef,
            const std::function<ThenPromiseType(PromiseType&&)>& functor)
    {
        auto thenPromise = new EPromiseMove<ThenPromiseType, PromiseType>(eObjectRef, functor);
        mExecuteThenFunctor = [=](PromiseType&& output){ thenPromise->execute(std::move(output)); };
        mThenPromisePtr = thenPromise;
        return thenPromise;
    }

private:
    bool mInitialized;
    UntypedEObjectRef mTargetEObjectRef;
    std::function<void(PromiseType&&)> mExecuteThenFunctor;
    std::function<PromiseType(ParamTypes&&...)> mExecuteFunctor;
    UntypedEObjectRef mCatchEObjectRef;
    std::function<void(std::exception_ptr)> mCatchFunctor;
    EDeletable *mThenPromisePtr;
};

}


#endif
