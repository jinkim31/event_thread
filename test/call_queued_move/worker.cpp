#include "worker.h"
#include "app.h"

void Worker::test(PassTester &&passTester)
{
    std::cout<<"== RECEIVED TESTER"<<std::endl;
    std::cout<<"== RECEIVED TESTER"<<std::endl;
    auto moved = std::move(passTester);
    std::cout<<"== TEST COMPLETED (TESTER ID: "<<moved.num()<<")"<<std::endl;
    EThread::stopMainThread();
}

