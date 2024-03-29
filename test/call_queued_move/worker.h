#ifndef EVENT_THREAD_WORKER_H
#define EVENT_THREAD_WORKER_H

#include <ethread.h>

using namespace ethr;

class App;

class Worker : public EObject
{
public:
    class PassTester
    {
    public:
        explicit PassTester(const int& num){mNum = num; std::cout<<"PassTester("<<num<<") constructed"<<std::endl;}
        PassTester(const PassTester& passTester){mNum = passTester.mNum; std::cout<<"PassTester("<<mNum<<") copied"<<std::endl;}
        PassTester(PassTester&& passTester) noexcept {mNum = passTester.mNum; std::cout<<"PassTester("<<mNum<<") moved"<<std::endl;}
        ~PassTester(){std::cout<<"PassTester("<<mNum<<") destructed"<<std::endl;}
        int num() const{return mNum;}
    private:
        int mNum;
    };

    void test2(PassTester &&passTester1, PassTester &&passTester2);
    void test1(PassTester &&passTester1);
};

#endif
