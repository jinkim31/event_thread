#include "secondThread.h"
#include "firstThread.h"

SecondThread::SecondThread()
{

}

void SecondThread::secondEventCallback(std::string str)
{
    std::cout<<"second thread received: "<<str<<std::endl;
}

void SecondThread::onStart()
{
    std::cout<<"first thread find "<<EventThread::findThread(firstThreadRef)<<std::endl;
}

void SecondThread::task()
{
    firstThreadRef.sharedResource<FirstThread::SharedResourceType>().manipulate([](FirstThread::SharedResourceType& shared){
        shared.a++;
        shared.b++;
    });

    EventThread::callInterthread(firstThreadRef, &FirstThread::firstEventCallback, std::string("hello from second thread!"));
}