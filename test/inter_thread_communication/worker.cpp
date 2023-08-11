#include "worker.h"
#include "app.h"

void Worker::setAppRef(const EObjectRef<App> &appRef)
{
    mAppRef = appRef;
}

void Worker::work(std::vector<int> &&numbers)
{
    for(int i=-0; i<numbers.size(); i++)
    {
        std::cout<<"worker is processing number "<<numbers[i]<<std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        mAppRef.callQueued(&App::progressReported, i+1, (int)numbers.size());
    }
}

