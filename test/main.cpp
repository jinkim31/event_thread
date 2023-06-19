#include <iostream>
#include "../event_thread/event_thread.h"
#include "../event_thread/event_thread_util.h"
#include <stdexcept>
#include <memory>

using namespace ethr;

class PromiseRejectedException : public std::runtime_error
{
public:
    explicit PromiseRejectedException(const std::string& msg) : runtime_error(msg.c_str())
    {};
};


class A : public EObject
{
public:
    int rand(int seed)
    {
        std::cout<<"rand "<<seed<<std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return 123;
    }
};

class B : public EObject
{
public:
    int add(int num)
    {
        std::cout<<"add "<<num<<std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return num+1;
    }
};

class C : public EObject
{
public:
    int mul(int num)
    {
        std::cout<<"mul "<<num<<std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return num*2;
    }
};

class Main : public EObject
{
public:
    ETimer timer;
    EThread threadA, threadB, threadC;
    A a;
    B b;
    C c;

    std::unique_ptr<EPromise<int, int>> promise;

    Main()
    {
        a.moveToThread(threadA);
        b.moveToThread(threadB);
        c.moveToThread(threadC);
        threadA.start();
        threadB.start();
        threadC.start();

        promise = std::make_unique<EPromise<int, int>>(&a, &A::rand);
        promise->then(&b, &B::add)
        .then(&c, &C::mul)
        .then(&b, &B::add)
        .then(&c, &C::mul)
        .then(&b, &B::add)
        .then(&c, &C::mul);

        timer.addTask(0, [&]{


            promise->execute(123);

            /*
            runQueued(&a, [&]
            {
                try
                {
                    auto r = a.rand(4);
                    runQueued(&b, [&]
                    {
                        try
                        {
                            b.print(r);
                        }
                        catch (const PromiseRejectedException &e)
                        {
                            std::cout << "ERROR " << e.what() << std::endl;
                        }
                    });
                }
                catch(const PromiseRejectedException &e)
                {
                    std::cout << "ERROR " << e.what() << std::endl;
                }
            });*/
        }, std::chrono::milliseconds(1000), 1);

        timer.start();
    }
};

int main()
{
    EThread mainThread;
    Main main;
    main.moveToThread(mainThread);
    mainThread.start();
    std::this_thread::sleep_for(std::chrono::seconds(20));
    mainThread.stop();
}
