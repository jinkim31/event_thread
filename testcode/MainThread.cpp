#include "MainThread.h"
#include "DerivedThread.h"

MainThread::MainThread() : EventThread("main")
{
    derivedThread.start();
    setLoopFreq(10);
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
}

void MainThread::onStart()
{

}

void MainThread::requestStop()
{
    std::cout<<"stop requested!"<<std::endl;
    stop();
}


