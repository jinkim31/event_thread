#include "firstThread.h"
#include "secondThread.h"

void FirstThread::onStart() 
{
    EventThread::findThread(secondThreadRef, "second");
}

void FirstThread::task()
{
    EventThread::callInterthread(secondThreadRef, &SecondThread::secondEventCallback, std::string("interthread!"));
}