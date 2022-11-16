#include "firstThread.h"
#include "secondThread.h"

void FirstThread::firstEventCallback(std::string str)
{
    //std::cout<<"first thread received: \""<<str<<"\" (tid:"<<gettid()<<")"<<std::endl;
}

void FirstThread::onStart()
{
    makeSharedResource<SharedResourceType>();
    manipulateSharedResource<SharedResourceType>([&](SharedResourceType& shared){
        shared.a = 100;
        shared.b = 100;
    });
}

void FirstThread::task()
{
    ethr::EventThread::callInterthreadNonspecific(&SecondThread::secondEventCallback, std::string("hello from first thread"));
    manipulateSharedResource<SharedResourceType>([&](SharedResourceType& shared){
        std::cout<<"a:"<<shared.a<<" b:"<<shared.b<<std::endl;
        shared.a++;
        shared.b++;
    });
}
