#include <ethread.h>
#include <etimer.h>
#include <epromise.h>

using namespace ethr;

class App : public EObject
{
public:
    class Worker : public EObject
    {
    public:
        EObjectRef<App> mAppRef;
        void call(int count)
        {
            std::cout<<"Worker called("<<count<<")"<<std::endl;
        }
    };

    App()
    {
        mCount = 0;
        mThread.setName("worker thread");
        mTimer.moveToThread(EThread::mainThread());
        mWorker.moveToThread(EThread::mainThread());

        std::cout<<"timer id: "<<mTimer.id()<<std::endl;
        std::cout<<"worker id: "<<mWorker.id()<<std::endl;
        std::cout<<"app id: "<<this->id()<<std::endl;
        mTimer.addTask(0, std::chrono::milliseconds(0), this->uref(), [&]
        {
            if(mCount++%2)
                mWorker.removeFromThread();
            else
            {
                mWorker.moveToThread(EThread::mainThread());
                mWorker.callQueued(&Worker::call, mCount);
            }
        });
        mThread.setLoopPeriod(std::chrono::milliseconds(0));
        mThread.start();
        mTimer.start();
    }
    ~App()
    {
        mTimer.removeFromThread();
        mWorker.removeFromThread();
        mThread.stop();
    }


private:
    ETimer mTimer;
    Worker mWorker;
    EThread mThread;
    int mCount;
};

int main()
{
    EThread mainThread("main thread");
    EThread::provideMainThread(mainThread);
    App app;
    app.moveToThread(mainThread);
    mainThread.start();
    app.removeFromThread();
}