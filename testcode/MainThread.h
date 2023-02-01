#include <eventThread.h>
#include "DerivedThread.h"

#ifndef EVENT_THREAD_MAINTHREAD_H
#define EVENT_THREAD_MAINTHREAD_H

using namespace ethr;

class MainThread : public EventThread
{
public:
    MainThread();
    ~MainThread();
events:
    void requestStop();
protected:
    void task() override;

    void onStart() override;
public:
    DerivedThread derivedThread;
};

#endif
