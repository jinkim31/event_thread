#include "secondThread.h"
#include "firstThread.h"

SecondThread::SecondThread()
{
    setLoopFreq(10);
}

void SecondThread::secondEventCallback(std::string str)
{
    std::cout<<"second thread received: "<<str<<std::endl;
}

void SecondThread::onStart()
{

}

void SecondThread::task()
{

}