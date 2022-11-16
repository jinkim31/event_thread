#include "firstThread.h"
#include "secondThread.h"

FirstThread::FirstThread() : EventThread("first")
{

}

void FirstThread::firstEventCallback(std::string str)
{

}

void FirstThread::onStart() 
{
    EventThread::findThread(secondThreadRef, "second");
}

void FirstThread::task()
{
    EventThread::callInterthread(secondThreadRef, &SecondThread::secondEventCallback, std::string("interthread!"));
}