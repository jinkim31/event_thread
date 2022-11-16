#include "firstThread.h"
#include "secondThread.h"
#include <unistd.h>

FirstThread::FirstThread()
{
    setEventHandleScheme(ethr::EventThread::EventHandleScheme::BEFORE_TASK);
}

void FirstThread::firstEventCallback(std::string str)
{
    std::cout<<"first thread received: \""<<str<<"\" (tid:"<<gettid()<<")"<<std::endl;
}

void FirstThread::task()
{
    ethr::EventThread::callInterthread(&SecondThread::secondEventCallback, std::string("hello from first thread"));
}
