#ifndef TESTTHREAD_H
#define TESTTHREAD_H

#include <eventThread.h>

using namespace ethr;

class SecondThread;

class FirstThread : public EventThread
{
public:
    struct SharedResourceType
    {
        int a;
        int b;
    };

    FirstThread();
events:

private:
    ThreadRef<SecondThread> secondThreadRef;

    virtual void onStart();
    virtual void task();
};

#endif // TESTTHREAD_H
