#include "worker.h"
#include "app.h"

void Worker::setAppRef(const EObjectRef<App> &appRef)
{
    mAppRef = appRef;
}

void Worker::work()
{
    for(int i=0; i<100; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        mAppRef.callQueued(&App::progressReported, i);
    }
}
