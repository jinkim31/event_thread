#ifndef EVENT_THREAD_BASETHREAD_H
#define EVENT_THREAD_BASETHREAD_H

#include <eventThread.h>

using namespace ethr;

class BaseThread : public EventThread
{
public:
    BaseThread();
    void basePrint();
private:
    int num;
};

#endif