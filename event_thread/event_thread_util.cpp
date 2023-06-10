#include "event_thread_util.h"

ethr::ELoopObserver::ELoopObserver()
{
    mIsObserving = false;
}

void ethr::ELoopObserver::start()
{
    mIsObserving = true;
    callQueued(&ELoopObserver::chainCallback);
}

void ethr::ELoopObserver::chainCallback()
{
    if(mIsObserving)
    {
        loopObserverCallback();
        callQueued(&ELoopObserver::chainCallback);
    }
}

void ethr::ELoopObserver::stop()
{
    mIsObserving = false;
}

void ethr::ETimer::loopObserverCallback()
{
    for(auto& [id, task] : mTasks)
    {
        if(task.nextTaskTime <= std::chrono::high_resolution_clock::now())
        {
            task.callback();
            task.nextTaskTime += task.period;
        }
    }
}

void ethr::ETimer::start()
{
    ELoopObserver::start();
}

void ethr::ETimer::stop()
{
    ELoopObserver::stop();
}

void ethr::ETimer::addTask(
        const int& id,
        const std::function<void(void)> &callback,
        const std::chrono::high_resolution_clock::duration &period)
{
    mTasks.insert({id, {callback, period, std::chrono::high_resolution_clock::now()}});
}

void ethr::ETimer::removeTask(const int &id)
{
    mTasks.erase(id);
}
