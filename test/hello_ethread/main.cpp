#include <iostream>
#include <ethread.h>
#include <etimer.h>

using namespace ethr;

class AcquisitionWorker : public EObject
{
public:
private:
};

class Controller : public EObject
{
public:
    void addWorker()
    {
        std::cout<<"added"<<std::endl;
        workers.emplace_back(std::make_unique<AcquisitionWorker>());
        threads.emplace_back(std::make_unique<EThread>());
        (*(workers.end() - 1))->addToThread(**(threads.end() - 1));
    }

    ~Controller()
    {
        for(auto& worker : workers)
            worker->removeFromThread();
    }
private:
    std::vector<std::unique_ptr<AcquisitionWorker>> workers;
    std::vector<std::unique_ptr<EThread>> threads;
};

class App : public EObject
{
public:
    App()
    {
        timer.addToThread(controllerThread);
        timer.addTask(1, std::chrono::milliseconds(100), [&]
        {
            controller.callQueued(&Controller::addWorker);
        }, 3);
        timer.addTask(0, std::chrono::milliseconds(400), [&]
        {
            EThread::stopMainThread();
        }, 1);
        timer.start();

        controller.addToThread(controllerThread);
        controllerThread.start();
    }
    ~App()
    {
        controller.removeFromThread();
        timer.removeFromThread();
        controllerThread.stop();
    }
private:
    ETimer timer;
    EThread controllerThread;
    Controller controller;
};

int main()
{
    EThread mainThread;
    EThread::provideMainThread(mainThread);
    App app;
    app.addToThread(mainThread);
    mainThread.start();
    app.removeFromThread();
}
