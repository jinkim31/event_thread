#include <ethread.h>
#include <etimer.h>
#include <epromise.h>

using namespace ethr;

class Worker : public EObject
{
public:
    void setId(int id)
    {
        mId = id;
    }

    int getId(int n)
    {
        std::cout<<"n: "<<n<<", ID: "<<mId<<std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return mId;
    }
private:
    int mId;
};

class App : public EObject
{
public:
    App()
    {
        for(int i=0; i<10; i++)
        {
            workers[i].setId(i);
            workers[i].addToThread(threads[i]);
            threads[i].start();
        }
        timer.addToThread(EThread::mainThread());
        timer.addTask(0, std::chrono::milliseconds(0), [&]
        {
            auto promise = new EPromise(&workers[0], &Worker::getId);
            promise
                    ->then(&workers[1], &Worker::getId)
                    ->then(&workers[2], &Worker::getId)
                    ->then(&workers[3], &Worker::getId)
                    ->then(&workers[4], &Worker::getId)
                    ->then(&workers[5], &Worker::getId)
                    ->then(&workers[6], &Worker::getId)
                    ->then(&workers[7], &Worker::getId)
                    ->then(&workers[8], &Worker::getId)
                    ->then(&workers[9], &Worker::getId);
            promise->execute(100);
        }, 1);
        timer.addTask(1, std::chrono::milliseconds(500), []
        {
            EThread::stopMainThread();
        }, 1);
        timer.start();

    }
    ~App()
    {
        timer.removeFromThread();
        for(auto & worker : workers)
            worker.removeFromThread();
        for(auto & thread : threads)
            thread.stop();
    }
private:
    ETimer timer;
    Worker workers[10];
    EThread threads[10];
};

int main()
{
    EThread mainThread("main");
    EThread::provideMainThread(mainThread);
    App app;
    app.addToThread(mainThread);
    mainThread.start();
    app.removeFromThread();
}