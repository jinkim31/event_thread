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

void DerivedThread::callback(ethr::SafeSharedPtr<std::array<int, 3>> arr)
{
    arr.readOnly([](ethr::SafeSharedPtr<std::array<int, 3>>::ReadOnlyPtr ptr){
        for(const auto& num : *ptr)
        {
            std::cout<<num<<std::endl;
        }
    });
}
