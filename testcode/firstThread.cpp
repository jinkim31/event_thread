#include "firstThread.h"
#include "secondThread.h"

FirstThread::FirstThread()
{
    setLoopFreq(10);
}

void FirstThread::onStart() 
{

}

void FirstThread::task()
{
    EventThread::callInterthread(&SecondThread::secondEventCallback, std::string("hello from first thread!"));
}