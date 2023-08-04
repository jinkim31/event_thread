#include "ethread.h"

ethr::EThread* ethr::EThread::mainEThreadPtr = nullptr;
int ethr::EObject::idCount = 0;
std::vector<std::pair<int, ethr::EObject*>> ethr::EObject::activeEObjectIds;
std::shared_mutex ethr::EObject::mutexActiveEObjectIds;

ethr::EObject::EObject()
{
    mId = EObject::idCount++;
    mThreadInAffinity = nullptr;
}

void ethr::EObject::addToThread(ethr::EThread& ethread)
{
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
}

ethr::EObject::~EObject()
{
    if(mThreadInAffinity)
        throw EObjectDestructedInThreadException(
                "EObject(" + std::string(typeid(*this).name()) + ") must be removed from thread in affinity before destruction.");
}

ethr::EThread & ethr::EObject::threadInAffinity()
{
    return *mThreadInAffinity;
}

int ethr::EObject::id()
{
    return mId;
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

void ethr::EThread::queueNewEvent(int eObjectId, const std::function<void()> &func)
{
    std::unique_lock<std::mutex> lock(mMutexEventQueue);
    if(std::find(mChildEObjectsIds.begin(), mChildEObjectsIds.end(), eObjectId) == mChildEObjectsIds.end())
        return;
    if(mEventQueue.size() < mEventQueueSize) mEventQueue.emplace_back(eObjectId, func);
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
    // lock mMutexEventHandling to prohibit event deletion when chile EObject::removeFromThread()
    std::unique_lock<std::mutex> executionLock(mMutexEventHandling);

    size_t nQueuedEvent = mEventQueue.size();
    for(int i=0; i<nQueuedEvent; i++)
    {
        // lock mMutexEventQueue to access event queue
        std::unique_lock<std::mutex> eventLock(mMutexEventQueue);
        auto func = mEventQueue.front().second;
        mEventQueue.pop_front();

        // unlocking mMutexEventQueue allows event push between event executions
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
    std::unique_lock<std::mutex> lock(mMutexChildObjects);
    std::unique_lock<std::shared_mutex> activeEObjectsLock(EObject::mutexActiveEObjectIds);

    mChildEObjectsIds.push_back(eObjectPtr->mId);
    EObject::activeEObjectIds.emplace_back(eObjectPtr->mId, eObjectPtr);
}

void ethr::EThread::removeChildEObject(EObject* eObjectPtr)
{
    std::unique_lock<std::mutex> childObjectsLock(mMutexChildObjects);
    std::unique_lock<std::mutex> executionLock(mMutexEventHandling);
    std::unique_lock<std::mutex> eventLock(mMutexEventQueue);
    std::unique_lock<std::shared_mutex> activeEObjectsLock(EObject::mutexActiveEObjectIds);

    std::erase_if(mChildEObjectsIds, [&](int id){return id == eObjectPtr->mId;});
    std::erase_if(mEventQueue, [&](std::pair<int, std::function<void(void)>>& pair){return pair.first==eObjectPtr->mId;});
    std::erase_if(EObject::activeEObjectIds,[&](std::pair<int, EObject*> pair){return pair.first == eObjectPtr->mId;});
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
