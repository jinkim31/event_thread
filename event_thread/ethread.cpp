#include "ethread.h"

ethr::EThread* ethr::EThread::mainEThreadPtr = nullptr;

ethr::EObject::EObject()
{
    mThreadInAffinity = nullptr;
}

void ethr::EObject::addToThread(ethr::EThread& ethread)
{
    std::shared_lock<std::shared_mutex> lock(mMutex);
    if(mThreadInAffinity)
        mThreadInAffinity->removeChildEObject(this);
    mThreadInAffinity = &ethread;
    mThreadInAffinity->addChildEObject(this);
}

void ethr::EObject::removeFromThread()
{
    std::shared_lock<std::shared_mutex> lock(mMutex);
    if(!mThreadInAffinity)
        return;
    mThreadInAffinity->removeChildEObject(this);
    mThreadInAffinity = nullptr;
}

ethr::EObject::~EObject()
{
    std::shared_lock<std::shared_mutex> lock(mMutex);
    if(mThreadInAffinity)
        throw EObjectDestructedInThreadException(
                "EObject(" + std::string(typeid(*this).name()) + ") must be removed from thread in affinity before destruction.");
}

ethr::EThread & ethr::EObject::threadInAffinity()
{
    return *mThreadInAffinity;
}


ethr::EThread::EThread(const std::string &name)
{
    mIsMain = false;
    mName = name;
    mEventQueueSize = 1000;
    mIsLoopRunning = false;
    mEventHandleScheme = EventHandleScheme::AFTER_TASK;
    mTaskPeriod = std::chrono::milliseconds(1);
}

ethr::EThread::~EThread()
{
    if(checkLoopRunningSafe())
    {
        std::cerr<<"[EThread] EThread::stop() must be called before it EThread destruction."<<std::endl;
    }
    stop();

    if(!mChildEObjects.empty())
        std::cerr<<"[EThread] EThread(" + mName + ") has child objects on destruction. "
                                                  "Use EObject::removeFromThread() before its destruction."<<std::endl;
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

void ethr::EThread::start()
{
    if(checkLoopRunningSafe())
        return;

    mIsLoopRunning = true;

    if(!mIsMain)
        mThread = std::thread(EThread::threadEntryPoint, this);
    else
        EThread::threadEntryPoint(this);
}

void ethr::EThread::stop()
{
    if(!checkLoopRunningSafe()) return;

    mMutexLoop.lock();
    mIsLoopRunning = false;
    mMutexLoop.unlock();
    if(!mIsMain)
    {
        if(mThread.joinable())
            mThread.join();
    }
}

void ethr::EThread::queueNewEvent(EObject *eObjectPtr, const std::function<void()> &func)
{
    std::unique_lock<std::mutex> lock(mMutexEvent);
    if(mEventQueue.size() < mEventQueueSize) mEventQueue.emplace_back(eObjectPtr, func);
}

void *ethr::EThread::threadEntryPoint(void *param)
{
    auto* ethreadPtr = (EThread*)param;
    ethreadPtr->mNextTaskTime = std::chrono::high_resolution_clock::now() + ethreadPtr->mTaskPeriod;
    ethreadPtr->runLoop();
    return nullptr;
}

void ethr::EThread::handleQueuedEvents()
{
    size_t nQueuedEvent = mEventQueue.size();
    for(int i=0; i<nQueuedEvent; i++)
    {
        // lock mutex and copy function at the front
        std::unique_lock<std::mutex> eventLock(mMutexEvent);
        auto func = mEventQueue.front().second;
        mEventQueue.pop_front();
        eventLock.unlock();

       // execute function
       func();
    }
}

void ethr::EThread::stopMainThread()
{
    if(mainEThreadPtr)
        mainEThreadPtr->stop();
    else
        throw MainEThreadNotAssignedException(
                "No EThread is assigned as main. "
                "Main EThread is assigned by calling EThread::start(true) with isMain=true argument.");
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

void ethr::EThread::addChildEObject(ethr::EObject *eObjectPtr)
{
    //std::cout<<"adding "<<eObjectPtr<<" from thread "<<this<<std::endl;
    std::unique_lock<std::mutex> lock(mMutexEObjects);
    mChildEObjects.push_back(eObjectPtr);
}

void ethr::EThread::removeChildEObject(EObject* eObjectPtr)
{
    //std::cout<<"removing "<<eObjectPtr<<" from thread "<<this<<std::endl;
    std::unique_lock<std::mutex> lock(mMutexEObjects);
    std::unique_lock<std::mutex> eventLock(mMutexEvent);
    std::erase_if(mChildEObjects, [&](EObject* ptr){return ptr==eObjectPtr;});
    std::erase_if(mEventQueue, [&](std::pair<EObject *, std::function<void(void)>>& pair){return pair.first==eObjectPtr;});
}

ethr::EThread & ethr::EThread::mainThread()
{
    return *EThread::mainEThreadPtr;
}

void ethr::EThread::provideMainThread(ethr::EThread &ethread)
{
    EThread::mainEThreadPtr = &ethread;
    ethread.mIsMain = true;
}
