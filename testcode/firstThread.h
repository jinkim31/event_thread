#ifndef TESTTHREAD_H
#define TESTTHREAD_H

#include "../event_thread/eventThread.h"

class SecondThread;

class FirstThread : public ethr::EventThread
{
public:
    struct SharedResourceType
    {
        int a;
        int b;
    };

    FirstThread() : EventThread("first")
    {

    }
events:
    void firstEventCallback(std::string str)
    {

    }
private:
    ethr::ThreadRef<SecondThread> secondThreadRef;

    virtual void onStart();
    virtual void task();
};

#endif // TESTTHREAD_H
