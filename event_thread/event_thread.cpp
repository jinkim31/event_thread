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
        mParentThread->notifyEObjectDestruction(this);
}

void ethr::EObject::notifyEThreadDestruction(ethr::EThread *eThreadPtr)
{
    if(mParentThread != eThreadPtr)
        std::cerr << "[EThread] EObject::notifyEThreadDestruction(ethr::EThread *eThreadPtr) "
                     "is called with a wrong EThread pointer."<<std::endl;
    mParentThread = nullptr;
}

ethr::EThread::EThread(const std::string& name)
{
    mName = name;
    mEventQueueSize = 1000;
    mIsLoopRunning = false;
    mEventHandleScheme = EventHandleScheme::AFTER_TASK;
    mTaskPeriod = std::chrono::milliseconds(1);
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
    for(auto iter = mEventQueue.begin(); iter != mEventQueue.end() ; iter++)
    {
        if(eObjectPtr == iter->first)
        {
            mEventQueue.erase(iter);
        }
    }
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
