#include "app.h"

App::App()
{
    mWorker.moveToThread(mWorkerThread);
    mWorker.setAppRef(this->ref<App>());
    mWorkerThread.start();

    mTimer.moveToThread(EThread::mainThread());
    mTimer.addTask(0, std::chrono::milliseconds(0), this->uref(), [&]
    {
        std::vector<int> numbers = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        mWorker.callQueuedMove(&Worker::work, std::move(numbers));
    }, 1);
    mTimer.start();
}

App::~App()
{
    mWorkerThread.stop();
    mWorker.removeFromThread();
    mTimer.removeFromThread();
}

void App::progressReported(const int progress, const int total)
{
    std::cout << "progress: " << progress << "/" << total << std::endl;
    if(progress == total)
        EThread::stopMainThread();
}