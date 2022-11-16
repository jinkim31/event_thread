#ifndef SECONDTHREAD_H
#define SECONDTHREAD_H

#include "../event_thread/eventThread.h"

class SecondThread : public EventThread
{
public:
    SecondThread();
events:
    void secondEventCallback(std::string str);
private:
    virtual void task() final;
};

#endif
