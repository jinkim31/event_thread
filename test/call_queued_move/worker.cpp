#include "worker.h"
#include "app.h"

void Worker::test2(PassTester &&passTester1, PassTester &&passTester2)
{
    std::cout<<"== RECEIVED TESTER"<<std::endl;
    auto moved1 = std::move(passTester1);
    auto moved2 = std::move(passTester2);
    std::cout<<"== TEST COMPLETED (TESTER1 ID: "<<moved1.num()<<" TESTER2 ID: "<<moved2.num()<<")"<<std::endl;
    EThread::stopMainThread();
}

void Worker::test1(Worker::PassTester &&passTester1)
{
    std::cout<<"== RECEIVED TESTER"<<std::endl;
    auto moved = std::move(passTester1);
    std::cout<<"== TEST COMPLETED (TESTER ID: "<<moved.num()<<")"<<std::endl;
    EThread::stopMainThread();
}

