#include "app.h"

App::App()
{
    mWorker.moveToThread(mWorkerThread);
    mWorkerThread.start();

    mTimer.moveToThread(EThread::mainThread());
    mTimer.addTask(0, std::chrono::milliseconds(0), this->uref(), [&]
    {
        std::cout << "== CREATING TESTER OBJECT" << std::endl;
        auto tester1 = Worker::PassTester(1);
        auto tester2 = Worker::PassTester(2);
        std::cout << "== TRANSMITTING" << std::endl;
        mWorker.callQueuedMove(&Worker::test2, std::move(tester1), std::move(tester2));
        //mWorker.callQueuedMove(&Worker::test1, std::move(tester1));
    }, 1);
    mTimer.start();
}

App::~App()
{
    mWorkerThread.stop();
    mWorker.removeFromThread();
    mTimer.removeFromThread();
}