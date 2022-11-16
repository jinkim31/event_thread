#ifndef SECONDTHREAD_H
#define SECONDTHREAD_H

#include "../event_thread/eventThread.h"

using namespace ethr;

class FirstThread;

class SecondThread : public EventThread
{
public:
    SecondThread();
events:
    void secondEventCallback(std::string str);
private:
    ThreadRef<FirstThread> firstThreadRef;
    virtual void onStart() final;
    virtual void task() final;
};

#endif
