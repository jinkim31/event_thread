#ifndef EVENT_THREAD_ETIMER_H
#define EVENT_THREAD_ETIMER_H

#include "ethread.h"

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
    void addTask(const int &id, const std::chrono::high_resolution_clock::duration &period, UntypedEObjectRef eObjectRef,
                 const std::function<void(void)> &callback, const int &timeToLive = -1);

    template <class EObjectType>
    void addTask(const int &id, const std::chrono::high_resolution_clock::duration &period, EObjectRef<EObjectType> eObjectRef,
                 void(EObjectType::*funcPtr)(), const int &timeToLive = -1)
    {
        mTasks.insert({
            id,
            {eObjectRef, [=]{((*(eObjectRef.eObjectUnsafePtr())).*funcPtr)();}, period, std::chrono::high_resolution_clock::now() + period, timeToLive}});
    }

    bool removeTask(const int &id);
private:
    struct Task
    {
        const UntypedEObjectRef eObjectRef;
        const std::function<void(void)> callback;
        const std::chrono::high_resolution_clock::duration period;
        std::chrono::high_resolution_clock::time_point nextTaskTime;
        int timeToLive; // -1: continuous
    };
    std::map<int, Task> mTasks; // map of {id : task}
    void loopObserverCallback() override;
};
}
#endif
