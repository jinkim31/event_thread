#include "ethread.h"

ethr::EThread* ethr::EThread::mainEThreadPtr = nullptr;
int ethr::EObject::idCount = 0;
std::map<int, ethr::EObject*> ethr::EObject::activeEObjectIds;
std::shared_mutex ethr::EObject::mutexActiveEObjectIds;

ethr::EObject::EObject()
{
    mId = EObject::idCount++;
    mThreadInAffinity = nullptr;
}

void ethr::EObject::moveToThread(ethr::EThread& ethread)
{
    onMovedToThread(ethread);

    if(mThreadInAffinity)
        mThreadInAffinity->removeChildEObject(this);
    mThreadInAffinity = &ethread;
    mThreadInAffinity->addChildEObject(this);
}

void ethr::EObject::removeFromThread()
{
    if(!mThreadInAffinity)
        return;
    mThreadInAffinity->removeChildEObject(this);
    mThreadInAffinity = nullptr;

    onRemovedFromThread();
}

ethr::EObject::~EObject()
{
    if(mThreadInAffinity)
        std::cerr<<"[EThread] EObject must be removed from thread in affinity before destruction."<<std::endl;
}

ethr::EThread & ethr::EObject::threadInAffinity()
{
    return *mThreadInAffinity;
}

ethr::UntypedEObjectRef ethr::EObject::uref()
{
    return UntypedEObjectRef(mId, this);
}

ethr::EThread::EThread(const std::string &name)
{
    mIsMain = false;
    mName = name;
    mEventQueueSize = 1000;
    mIsLoopRunning = false;
    mEventHandleScheme = EventHandleScheme::AFTER_TASK;
    mLoopPeriod = std::chrono::milliseconds(1);
    mNEventQueueReservedForHandle = 0;
}

ethr::EThread::~EThread()
{
    if(checkLoopRunningSafe())
    {
        std::cerr<<"[EThread] EThread::stop() must be called before it EThread destruction."<<std::endl;
    }
    stop();

    if(!mChildEObjectsIds.empty())
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
    mLoopPeriod = period;
}

void ethr::EThread::setLoopFreq(const unsigned int& freq)
{
    if(checkLoopRunningSafe()) return;
    mLoopPeriod = std::chrono::seconds(1) / freq;
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

void ethr::EThread::queueNewEvent(int eObjectId, const std::function<void()> &func)
{
    std::cout<<"E"<<std::endl;
    std::unique_lock<std::mutex> lock(mMutexEventQueue);
    if(std::find(mChildEObjectsIds.begin(), mChildEObjectsIds.end(), eObjectId) == mChildEObjectsIds.end())
        return;
    if(mEventQueue.size() < mEventQueueSize) mEventQueue.emplace_back(eObjectId, func);
    std::cout<<"F"<<std::endl;
}

void *ethr::EThread::threadEntryPoint(void *param)
{
    auto* ethreadPtr = (EThread*)param;
    ethreadPtr->mNextTaskTime = std::chrono::high_resolution_clock::now() + ethreadPtr->mLoopPeriod;
    ethreadPtr->runLoop();
    return nullptr;
}

void ethr::EThread::handleQueuedEvents()
{
    // lock mMutexEventHandling to prohibit event deletion when chile EObject::removeFromThread()
    if(mNEventQueueReservedForHandle == 0)
        // mNEventQueueReservedForHandle == 0 means its the first call in the recursion
        std::unique_lock<std::mutex> executionLock(mMutexEventHandling);


    size_t nQueuedEvent = mEventQueue.size();
    size_t nHandlingEventsHere = nQueuedEvent - mNEventQueueReservedForHandle;
    size_t iHandleStartEvent = mNEventQueueReservedForHandle;
    mNEventQueueReservedForHandle += nHandlingEventsHere;

    for(int i=0; i<nHandlingEventsHere; i++)
    {
        // lock mMutexEventQueue to access event queue
        std::unique_lock<std::mutex> eventLock(mMutexEventQueue);
        auto func = mEventQueue[iHandleStartEvent + i].second;

        // unlocking mMutexEventQueue allows event push between event executions
        eventLock.unlock();

       // execute function
       func();
    }

    for(int i=0; i<nHandlingEventsHere; i++)
        mEventQueue.pop_front();

    mNEventQueueReservedForHandle -= nHandlingEventsHere;
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
        mNextTaskTime += mLoopPeriod;

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
    std::unique_lock<std::mutex> lock(mMutexChildObjects);
    std::unique_lock<std::shared_mutex> activeEObjectsLock(EObject::mutexActiveEObjectIds);

    mChildEObjectsIds.push_back(eObjectPtr->mId);
    EObject::activeEObjectIds.insert({eObjectPtr->mId, eObjectPtr});
}

void ethr::EThread::removeChildEObject(EObject* eObjectPtr)
{
    std::unique_lock<std::mutex> childObjectsLock(mMutexChildObjects);
    std::unique_lock<std::mutex> executionLock(mMutexEventHandling);
    std::unique_lock<std::mutex> eventLock(mMutexEventQueue);
    std::unique_lock<std::shared_mutex> activeEObjectsLock(EObject::mutexActiveEObjectIds);

    std::erase_if(mChildEObjectsIds, [&](int id){return id == eObjectPtr->mId;});
    std::erase_if(mEventQueue, [&](std::pair<int, std::function<void(void)>>& pair){return pair.first==eObjectPtr->mId;});
    EObject::activeEObjectIds.erase(eObjectPtr->mId);
}

ethr::EThread & ethr::EThread::mainThread()
{
    if(EThread::mainEThreadPtr == nullptr)
        throw std::runtime_error(
                "[EThread] EThread::mainThread() is called but no main thread is assigned. "
                "Use EThread::provideMainThread() to assign a main thread.");
    return *EThread::mainEThreadPtr;
}

void ethr::EThread::provideMainThread(ethr::EThread &ethread)
{
    EThread::mainEThreadPtr = &ethread;
    ethread.mIsMain = true;
}

void ethr::EThread::waitForEventHandleCompletion()
{
    while(true)
    {
        std::unique_lock<std::mutex> lock(mMutexEventQueue);
        if(mEventQueue.empty())
            return;
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
