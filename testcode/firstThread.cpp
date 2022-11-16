#include "firstThread.h"
#include "secondThread.h"

FirstThread::FirstThread() : EventThread("first")
{

}

void FirstThread::firstEventCallback(std::string str)
{
    std::cout<<"first thread received: "<<str<<std::endl;
}

void FirstThread::onStart() 
{
    EventThread::findThread(secondThreadRef, "second");
}

void FirstThread::task()
{
    EventThread::callInterthread(secondThreadRef, &SecondThread::secondEventCallback, std::string("hello from first thread!"));
}