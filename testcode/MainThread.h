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
    void requestStop();
protected:
    void task() override;

    void onStart() override;
private:
    DerivedThread derivedThread;
    ethr::SafeSharedPtr<std::array<int, 3>> arr;
};

#endif
