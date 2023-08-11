#ifndef EVENT_THREAD_APP_H
#define EVENT_THREAD_APP_H

#include <ethread.h>
#include <etimer.h>
#include "worker.h"

using namespace ethr;

class App : public EObject
{
public:
    App();
    ~App();
private:
    ETimer mTimer;
    EThread mWorkerThread;
    Worker mWorker;
};


#endif
