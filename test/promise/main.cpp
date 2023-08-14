#include <ethread.h>
#include <etimer.h>
#include <epromise.h>

using namespace ethr;

class Worker : public EObject
{
public:
    void setMultiplier(int multiplier)
    {
        mMultiplier = multiplier;
    }

    int multiply(int n)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout<<n<<"*"<<mMultiplier<<"="<<n*mMultiplier<<std::endl;
        return n*mMultiplier;
    }
private:
    int mMultiplier;
};

class App : public EObject
{
public:
    App()
    {
        for(int i=0; i<10; i++)
        {
            workers[i].setMultiplier(i+1);
            workers[i].moveToThread(threads[i]);
            threads[i].start();
        }
        mTimer.moveToThread(EThread::mainThread());
        mTimer.addTask(0, std::chrono::milliseconds(0), this->uref(), [&]
        {
            auto promise = new EPromise(workers[0].ref<Worker>(), &Worker::multiply);
            promise
                    ->then(workers[1].ref<Worker>(), &Worker::multiply)
                    ->then(workers[2].ref<Worker>(), &Worker::multiply)
                    ->then(workers[3].ref<Worker>(), &Worker::multiply)
                    ->then(workers[4].ref<Worker>(), &Worker::multiply)
                    ->then(workers[5].ref<Worker>(), &Worker::multiply)
                    ->then(workers[6].ref<Worker>(), &Worker::multiply)
                    ->then(workers[7].ref<Worker>(), &Worker::multiply)
                    ->then(workers[8].ref<Worker>(), &Worker::multiply)
                    ->then(workers[9].ref<Worker>(), &Worker::multiply);
            promise->execute(2);
        }, 1);
        mTimer.addTask(1, std::chrono::milliseconds(6000), this->uref(), []
        {
            EThread::stopMainThread();
        }, 1);
        mTimer.start();
    }
    ~App()
    {
        mTimer.removeFromThread();
        for(auto & worker : workers)
            worker.removeFromThread();
        for(auto & thread : threads)
            thread.stop();
    }
private:
    ETimer mTimer;
    Worker workers[10];
    EThread threads[10];
};

int main()
{
    EThread mainThread("main");
    EThread::provideMainThread(mainThread);
    App app;
    app.moveToThread(mainThread);
    mainThread.start();
    app.removeFromThread();
}