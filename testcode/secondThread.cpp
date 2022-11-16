#include <unistd.h>
#include "secondThread.h"
#include "firstThread.h"

void SecondThread::secondEventCallback(std::string str)
{
    //std::cout<<"second thread received: \""<<str<<"\" (tid:"<<gettid()<<")"<<std::endl;
}

void SecondThread::onStart()
{

}

void SecondThread::task()
{
    //ethr::EventThread::callInterthreadNonspecific(&FirstThread::firstEventCallback, std::string("hello from second thread"));
}