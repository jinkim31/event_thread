#ifndef TESTTHREAD_H
#define TESTTHREAD_H

#include "../event_thread/eventThread.h"
#include "../event_thread/threadRef.h"

class SecondThread;

class FirstThread : public ethr::EventThread
{
events:
    void firstEventCallback(std::string str);
private:
    struct SharedResourceType
    {
        int a;
        int b;
    };
    
    ThreadRef<SecondThread> secondThread;
    virtual void onStart() final;
    virtual void task() final;
};

#endif // TESTTHREAD_H
