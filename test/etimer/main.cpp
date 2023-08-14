#include <ethread.h>
#include <etimer.h>

using namespace ethr;

class App : public EObject
{
public:
    App()
    {
        mTimerCount = 0;

        mTimer.moveToThread(EThread::mainThread());
        mTimer.addTask(0, std::chrono::milliseconds(500),
                       this->ref<App>(), &App::timerCallback, 3);
        mTimer.addTask(1, std::chrono::milliseconds(2000), this->uref(), [&]
        {
            std::cout<<"Terminating. Timer count:"<<mTimerCount<<std::endl;
            EThread::stopMainThread();
        }, 1);
        mTimer.start();
    }
    ~App()
    {
        mTimer.stop();
        mTimer.removeFromThread();
    }
private:
    ETimer mTimer;
    int mTimerCount;
    void timerCallback()
    {
        std::cout<<"Timer! Timer count:"<<mTimerCount++<<std::endl;
    }
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