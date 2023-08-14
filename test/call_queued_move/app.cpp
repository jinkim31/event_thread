#include "app.h"

App::App()
{
    mWorker.moveToThread(mWorkerThread);
    mWorkerThread.start();

    mTimer.moveToThread(EThread::mainThread());
    mTimer.addTask(0, std::chrono::milliseconds(0), this->uref(), [&]
    {
        std::cout << "== CREATING TESTER OBJECT" << std::endl;
        auto tester = Worker::PassTester(123);
        std::cout << "== TRANSMITTING" << std::endl;
        mWorker.callQueuedMove(&Worker::test, std::move(tester));
    }, 1);
    mTimer.start();
}

App::~App()
{
    mWorkerThread.stop();
    mWorker.removeFromThread();
    mTimer.removeFromThread();
}