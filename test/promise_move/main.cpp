#include <ethread.h>
#include <etimer.h>
#include <epromise.h>

using namespace ethr;

class PassTester
{
public:
    explicit PassTester(const int& num){mNum = num; std::cout<<"PassTester("<<num<<") constructed"<<std::endl;}
    PassTester(const PassTester& passTester){mNum = passTester.mNum; std::cout<<"PassTester("<<mNum<<") copied"<<std::endl;}
    PassTester(PassTester&& passTester) noexcept {mNum = passTester.mNum; std::cout<<"PassTester("<<mNum<<") moved"<<std::endl;}
    ~PassTester(){std::cout<<"PassTester("<<mNum<<") destructed"<<std::endl;}
    int num() const{return mNum;}
private:
    int mNum;
};

class Worker : public EObject
{
public:
    PassTester echo(PassTester&& tester)
    {
        std::cout<<"echo"<<std::endl;
        return std::move(tester);
    }
    PassTester echo2(PassTester&& tester)
    {
        std::cout<<"echo2"<<std::endl;
        return std::move(tester);
    }
};

class App : public EObject
{
public:
    App()
    {
        mWorker.moveToThread(mThread);
        mTimer.moveToThread(EThread::mainThread());
        mTimer.addTask(0, std::chrono::milliseconds(0), this->uref(), [&]
        {
            auto promise = new EPromiseMove(mWorker.ref<Worker>(), &Worker::echo);
            promise
            ->then(mWorker.ref<Worker>(), &Worker::echo2)
            ->then(mWorker.ref<Worker>(), &Worker::echo)
            ->then(mWorker.ref<Worker>(), &Worker::echo2)
            ->then(mWorker.ref<Worker>(), &Worker::echo)
            ->then(mWorker.ref<Worker>(), &Worker::echo2)
            ->then<int>(this->uref(), [](PassTester&& tester){
                std::cout<<"test done"<<std::endl;
                return 0;});
            PassTester tester(123);

            promise->execute(std::move(tester));
        }, 1);
        mTimer.addTask(1, std::chrono::milliseconds(2000), this->uref(), []
        {
            EThread::stopMainThread();
        }, 1);
        mTimer.start();
        mThread.start();
    }
    ~App()
    {
        mThread.stop();
        mWorker.removeFromThread();
        mTimer.removeFromThread();
    }
private:
    EThread mThread;
    Worker mWorker;
    ETimer mTimer;
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