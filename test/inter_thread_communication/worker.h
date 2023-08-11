#ifndef EVENT_THREAD_WORKER_H
#define EVENT_THREAD_WORKER_H

#include <ethread.h>

using namespace ethr;

class App;

class Worker : public EObject
{
public:
    void setAppRef(const EObjectRef<App>& appRef);
    void work(std::vector<int> &&numbers);
private:
    EObjectRef<App> mAppRef;
};

#endif
