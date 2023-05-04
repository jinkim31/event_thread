#include "eventThread.h"

std::map<std::thread::id, ethr::EThread*> ethr::EThread::ethreads;

ethr::EThreadObject::EThreadObject(EThread* ethreadPtr)
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

void ethr::EThreadObject::moveToThread(ethr::EThread& ethread)
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
     * stop() should be called explicitly before thread destruction.
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

    isInNewThread = makeNewThread;

    if(makeNewThread)
    {
#ifdef ETHREAD_USE_PTHREAD
        pthread_create(&mThread, NULL, EThread::threadEntryPoint, this);
#else
        mThread = std::thread(EThread::threadEntryPoint, this);
#endif
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
    if(isInNewThread)
    {
#ifdef ETHREAD_USE_PTHREAD
        void* ret;
        pthread_join(mThread, &ret);
#else
        if(mThread.joinable())
            mThread.join();
#endif
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
    while(!mEventQueue.empty())
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
    std::unique_lock<std::mutex> lock(mMutexLoop);
    mIsLoopRunning = true;
    lock.unlock();

    onStart();

    while(checkLoopRunningSafe())
    {
        while(std::chrono::high_resolution_clock::now() < mNextTaskTime);
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