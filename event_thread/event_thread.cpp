#include "event_thread.h"

std::map<std::thread::id, ethr::EThread*> ethr::EThread::ethreads;

ethr::EObject::EObject(EThread* ethreadPtr)
{
    mParentThread = nullptr;
    if(ethreadPtr != nullptr)
        return;
    //std::cout<<"looking for thread with tid: "<<std::this_thread::get_id()<<std::endl;
    auto foundThread = EThread::ethreads.find(std::this_thread::get_id());
    if(foundThread == EThread::ethreads.end())
        return;
    //std::cout<<"EThread found with name "<<foundThread->second->mName<<std::endl;
    mParentThread = foundThread->second;
}

void ethr::EObject::moveToThread(ethr::EThread& ethread)
{
    mParentThread = &ethread;
}

ethr::EThread::EThread(const std::string& name)
{
    mName = name;
    mEventQueueSize = 1000;
    mIsLoopRunning = false;
    mEventHandleScheme = EventHandleScheme::AFTER_TASK;
    mTaskPeriod = std::chrono::high_resolution_clock::duration::zero();
    ethreads.insert({std::this_thread::get_id(), this});
}

ethr::EThread::~EThread()
{
    /*
     * stop() should be called explicitly before EThread destruction.
     * otherwise crashes like following will occur:
     *
     * pure virtual method called
     * terminate called without an active exception
     *
     * stop() is here just for the context and it's not safe to terminate a ethread with it
     * since any virtual function overridden by the derived class is destructed prior to EThread itself.
     */
    stop();
}

bool ethr::EThread::checkLoopRunningSafe()
{
    std::unique_lock<std::mutex> lock(mMutexLoop);
    return mIsLoopRunning;
}

void ethr::EThread::setName(const std::string& name)
{
    mName = name;
}

void ethr::EThread::setLoopPeriod(std::chrono::duration<long long int, std::nano> period)
{
    if(checkLoopRunningSafe()) return;
    mTaskPeriod = period;
}

void ethr::EThread::setLoopFreq(const unsigned int& freq)
{
    if(checkLoopRunningSafe()) return;
    mTaskPeriod = std::chrono::seconds(1) / freq;
}

void ethr::EThread::setEventHandleScheme(EventHandleScheme scheme)
{
    if(checkLoopRunningSafe()) return;
    mEventHandleScheme = scheme;
}

void ethr::EThread::start(bool makeNewThread)
{
    if(checkLoopRunningSafe()) return;

    mIsInNewThread = makeNewThread;

    if(makeNewThread)
    {
        mIsLoopRunning = true;
        mThread = std::thread(EThread::threadEntryPoint, this);
    }
    else
    {
        EThread::threadEntryPoint(this);
    }
}

void ethr::EThread::stop()
{
    if(!checkLoopRunningSafe()) return;

    mMutexLoop.lock();
    mIsLoopRunning = false;
    mMutexLoop.unlock();
    if(mIsInNewThread)
    {
        if(mThread.joinable())
            mThread.join();
    }
}

void ethr::EThread::queueNewEvent(const std::function<void ()> &func)
{
    std::unique_lock<std::mutex> lock(mMutexEvent);
    if(mEventQueue.size() < mEventQueueSize) mEventQueue.push(func);
}

void *ethr::EThread::threadEntryPoint(void *param)
{
    EThread* ethreadPtr = (EThread*)param;
    ethreadPtr->mNextTaskTime = std::chrono::high_resolution_clock::now() + ethreadPtr->mTaskPeriod;
    ethreadPtr->runLoop();
    return nullptr;
}

void ethr::EThread::handleQueuedEvents()
{
    int nQueuedEvent = mEventQueue.size();
    for(int i=0; i<nQueuedEvent; i++)
    {
        // lock mutex and copy function at the front
        std::unique_lock<std::mutex> eventLock(mMutexEvent);
        auto func = mEventQueue.front();
        mEventQueue.pop();
        eventLock.unlock();

       // execute function
       func();
    }
}

void ethr::EThread::runLoop()
{
    onStart();

    while(checkLoopRunningSafe())
    {
        std::this_thread::sleep_for(mNextTaskTime - std::chrono::high_resolution_clock::now());
        mNextTaskTime += mTaskPeriod;

        switch(mEventHandleScheme)
        {
        case EventHandleScheme::AFTER_TASK:
        {
            task();
            handleQueuedEvents();
            break;
        }
        case EventHandleScheme::BEFORE_TASK:
        {
            handleQueuedEvents();
            task();
            break;
        }
        case EventHandleScheme::USER_CONTROLLED:
        {
            task();
            break;
        }
        }
    }

    onTerminate();
}