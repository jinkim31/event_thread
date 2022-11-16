#ifndef SECONDTHREAD_H
#define SECONDTHREAD_H

#include "../event_thread/eventThread.h"

class FirstThread;

class SecondThread : public ethr::EventThread
{
public:
    SecondThread():EventThread("second")
    {

    }
events:
    void secondEventCallback(std::string str)
    {
        std::cout<<"second received: "<<str<<std::endl;
    }
private:
    virtual void onStart() final
    {
    }

    virtual void task() final
    {

    }
};

#endif
