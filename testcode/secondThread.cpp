#include "secondThread.h"

SecondThread::SecondThread():EventThread("second")
{

}

void SecondThread::secondEventCallback(std::string str)
{
    std::cout<<"second received: "<<str<<std::endl;
}

void SecondThread::onStart()
{
}

void SecondThread::task()
{

}