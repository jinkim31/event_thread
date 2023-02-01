#include "DerivedThread.h"
#include "MainThread.h"

DerivedThread::DerivedThread()
{
    setLoopFreq(10);
}

void DerivedThread::task()
{

}

void DerivedThread::onStart()
{

}

void DerivedThread::derivedPrint()
{
    std::cout<<"derived print"<<std::endl;
}
