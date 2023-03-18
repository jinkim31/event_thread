#include "MainThread.h"
#include "DerivedThread.h"

MainThread::MainThread() : EventThread("main")
{
    derivedThread.start();
    setLoopPeriod(std::chrono::milliseconds(500));
    arr = ethr::SafeSharedPtr<std::array<int, 3>>(std::make_shared<std::array<int, 3>>());
    arr.readWrite([&](ethr::SafeSharedPtr<std::array<int, 3>>::ReadWritePtr ptr){
        ptr->at(0) = 10;
        ptr->at(1) = 20;
        ptr->at(2) = 30;
    });
}

MainThread::~MainThread()
{
    derivedThread.stop();
}

void MainThread::task()
{
    static int count = 0;
    if(count++>5)
        stop();

    EventThread::callInterthread(&DerivedThread::basePrint);
    EventThread::callInterthread(&DerivedThread::derivedPrint);
    EventThread::callInterthread(&DerivedThread::callback, arr);
}

void MainThread::onStart()
{

}

void MainThread::requestStop()
{
    std::cout<<"stop requested!"<<std::endl;
    stop();
}
