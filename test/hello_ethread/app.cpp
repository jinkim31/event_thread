#include "app.h"

App::App()
{
    mWorker.moveToThread(mWorkerThread);
    mWorker.setAppRef(this->ref<App>());
    mWorkerThread.start();

    mTimer.moveToThread(EThread::mainThread());
    mTimer.addTask(0, std::chrono::milliseconds(0), [&]{
        mWorker.callQueued(&Worker::work);
    }, 1);
    mTimer.start();
}

App::~App()
{
    mWorkerThread.stop();
    mWorker.removeFromThread();
    mTimer.removeFromThread();
}

void App::progressReported(int progress)
{
    std::cout<<"progress: "<<progress<<"/99"<<std::endl;
    if(progress == 99)
        EThread::stopMainThread();
}
