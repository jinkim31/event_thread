#ifndef EVENT_THREAD_DERIVEDTHREAD_H
#define EVENT_THREAD_DERIVEDTHREAD_H

#include "BaseThread.h"

class DerivedThread : public BaseThread
{
public:
    DerivedThread();
events:
    void derivedPrint();
    void callback(ethr::SafeSharedPtr<std::array<int, 3>> arr);
protected:
    void task() override;

    void onStart() override;

private:
};

#endif
