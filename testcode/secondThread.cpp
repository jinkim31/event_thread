#include "firstThread.h"
#include "secondThread.h"
#include <unistd.h>

SecondThread::SecondThread()
{

}

void SecondThread::secondEventCallback(std::string str)
{
    std::cout<<"second thread received: \""<<str<<"\" (tid:"<<gettid()<<")"<<std::endl;
}

void SecondThread::task()
{
    EventThread::callInterthread(&FirstThread::firstEventCallback, std::string("hello from second thread"));
}
