#ifndef TESTTHREAD_H
#define TESTTHREAD_H

#include "../event_thread/eventThread.h"

class FirstThread : public ethr::EventThread
{
public:
    FirstThread();
events:
    void firstEventCallback(std::string str);
private:
    virtual void task() final;
};

#endif // TESTTHREAD_H
