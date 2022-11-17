#include "firstThread.h"
#include "secondThread.h"

FirstThread::FirstThread()
{

}

void FirstThread::firstEventCallback(std::string str)
{
    int a;
    sharedResource<SharedResourceType>().manipulate([&](SharedResourceType& shared){
        a = shared.a;
    });
    std::cout<<"first thread received: "<<str<<" a:"<<a<<std::endl;
}

void FirstThread::onStart() 
{
    EventThread::findThread(secondThreadRef);

    makeSharedResource<SharedResourceType>();
    sharedResource<SharedResourceType>().manipulate([&](SharedResourceType& shared){
        shared.a = 100;
        shared.b = 200;
    });
}

void FirstThread::task()
{
    EventThread::callInterthread(secondThreadRef, &SecondThread::secondEventCallback, std::string("hello from first thread!"));
}