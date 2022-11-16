#include "secondThread.h"
#include "firstThread.h"

SecondThread::SecondThread():EventThread("second")
{

}

void SecondThread::secondEventCallback(std::string str)
{
    std::cout<<"second thread received: "<<str<<std::endl;
}

void SecondThread::onStart()
{
    EventThread::findThread(firstThreadRef, "first");
}

void SecondThread::task()
{
    EventThread::callInterthread(firstThreadRef, &FirstThread::firstEventCallback, std::string("hello from second thread!"));
}