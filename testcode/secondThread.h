#ifndef SECONDTHREAD_H
#define SECONDTHREAD_H

#include "../event_thread/eventThread.h"

class FirstThread;

class SecondThread : public ethr::EventThread
{
events:
    void secondEventCallback(std::string str);
private:
    virtual void onStart() final;
    virtual void task() final;
};

#endif
