#include "ethread.h"

std::map<std::thread::id, ethr::EThread*> ethr::EThread::eThreads;
ethr::EThread* ethr::EThread::mainEThreadPtr = nullptr;

ethr::EObject::EObject(EThread* ethreadPtr)
{
    mParentThread = ethreadPtr;
    // return if ethread is explicitly assigned
    if(ethreadPtr != nullptr)
    {
        mParentThread->addChildEObject(this);
        return;
    }
    // find main ethread
    auto foundThread = EThread::eThreads.find(std::this_thread::get_id());
    // return if not found
    if(foundThread == EThread::eThreads.end())
    {
        return;
    }
    // assign found ethread
    mParentThread = foundThread->second;
    mParentThread->addChildEObject(this);
}

void ethr::EObject::moveToThread(ethr::EThread& ethread)
{
    if(mParentThread != nullptr)
        mParentThread->removeChildEObject(this);
    ethread.addChildEObject(this);
    mParentThread = &ethread;
}

ethr::EObject::~EObject()
{
    if(mParentThread != nullptr)
    {
        mParentThread->notifyEObjectDestruction(this);
    }
}

void ethr::EObject::notifyEThreadDestruction(ethr::EThread *eThreadPtr)
{
    if(mParentThread != eThreadPtr)
    {
        std::cerr << "[EThread] EObject::notifyEThreadDestruction(ethr::EThread *eThreadPtr) "
                     "is called with a wrong EThread pointer." << std::endl;
        throw std::runtime_error("!!!!");
    }
    mParentThread = nullptr;
}

ethr::EThread::EThread(const std::string& name)
{
    mName = name;
    mEventQueueSize = 1000;
    mIsLoopRunning = false;
    mEventHandleScheme = EventHandleScheme::AFTER_TASK;
    mTaskPeriod = std::chrono::milliseconds(1);
    eThreads.insert({std::this_thread::get_id(), this});
}

ethr::EThread::~EThread()
{
    /*
     * stop() should be called explicitly before EThread destruction.
     * otherwise crashes like following will occur:
     *
     * "
     * pure virtual method called
     * terminate called without an active exception
     * "
     *
     * stop() is here just for the context and it's not safe to terminate a ethread with it
     * since any virtual function overridden by the derived class is destructed prior to EThread itself.
     */
    if(checkLoopRunningSafe())
    {
        std::cerr<<"EThread::stop() should be called before it EThread destruction."<<std::endl;
    }
    stop();

    for(const auto& childEObject : mChildEObjects)
        childEObject->notifyEThreadDestruction(this);
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

void ethr::EThread::start(bool isMain)
{
    if(checkLoopRunningSafe())
        return;
    if(isMain && mainEThreadPtr)
        throw MainEThreadAlreadyAssignedException("You can only assign one EThread as main.");

    mIsMain = isMain;
    mIsLoopRunning = true;

    if(!mIsMain)
    {
        mThread = std::thread(EThread::threadEntryPoint, this);
    }
    else
    {
        mainEThreadPtr = this;
        EThread::threadEntryPoint(this);
    }
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

void ethr::EThread::notifyEObjectDestruction(ethr::EObject *eObjectPtr)
{
    std::unique_lock<std::mutex> eventLock(mMutexEvent);
    std::erase_if(mEventQueue, [&](std::pair<EObject *, std::function<void(void)>>& pair){return pair.first==eObjectPtr;});
    std::erase_if(mChildEObjects, [&](EObject* ptr){return ptr==eObjectPtr;});
}

void ethr::EThread::addChildEObject(ethr::EObject *eObjectPtr)
{
    mChildEObjects.push_back(eObjectPtr);
}

void ethr::EThread::removeChildEObject(EObject* eObjectPtr)
{
    mChildEObjects.erase(
            std::remove(mChildEObjects.begin(), mChildEObjects.end(), eObjectPtr),
            mChildEObjects.end());
}
