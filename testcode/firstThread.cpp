#include "firstThread.h"
#include "secondThread.h"
#include <unistd.h>

FirstThread::FirstThread()
{

}

void FirstThread::firstEventCallback(std::string str)
{
    std::cout<<"first thread received: \""<<str<<"\" (tid:"<<gettid()<<")"<<std::endl;
}

void FirstThread::task()
{
    EventThread::callInterthread(&SecondThread::secondEventCallback, std::string("hello from first thread"));
}
