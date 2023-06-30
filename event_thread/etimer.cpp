#include "etimer.h"

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
    std::vector<std::map<int, Task>::iterator> ttlExpiredTasks;

    for(auto iter = mTasks.begin(); iter != mTasks.end(); iter++)
    {
        if(iter->second.nextTaskTime <= std::chrono::high_resolution_clock::now())
        {
            iter->second.callback();
            iter->second.nextTaskTime += iter->second.period;

            if(--iter->second.timeToLive == 0)
                ttlExpiredTasks.push_back(iter);
        }
    }

    for(const auto& iter : ttlExpiredTasks)
        mTasks.erase(iter);
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
        const int &id,
        const std::function<void(void)> &callback,
        const std::chrono::high_resolution_clock::duration &period,
        const int &timeToLive)
{
    mTasks.insert({
        id,
        {callback, period, std::chrono::high_resolution_clock::now() + period, timeToLive}});
}

bool ethr::ETimer::removeTask(const int &id)
{
    bool taskExists = mTasks.find(id) != mTasks.end();
    mTasks.erase(id);
    return taskExists;
}
