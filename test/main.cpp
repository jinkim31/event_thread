#include <iostream>
#include "../event_thread/event_thread.h"
#include "../event_thread/event_thread_util.h"
#include <stdexcept>
#include <memory>

using namespace ethr;

class Adder : public EObject
{
public:
    int add(int num)
    {
        int res = num + 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout<<"+ : "<<num<<"->"<<res<<std::endl;
        return res;
    }
};

class Subtractor : public EObject
{
public:
    int subtract(int num)
    {
        int res = num - 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout<<"- : "<<num<<"->"<<res<<std::endl;
        return res;
    }
};

class Multiplier : public EObject
{
public:
    int multiply(int num)
    {
        int res = num * 2;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout<<"* : "<<num<<"->"<<res<<std::endl;
        return res;
    }
};

class Divider : public EObject
{
public:
    int divide(int num)
    {
        int res = num / 2;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout<<"/ : "<<num<<"->"<<res<<std::endl;
        return res;
    }
};

class Main : public EObject
{
public:
    ETimer timer;
    EThread thread1, thread2, thread3, thread4;
    Adder adder;
    Subtractor subtractor;
    Multiplier multiplier;
    Divider divider;

    EPromise<int, int> promise;

    Main()
    {
        adder.moveToThread(thread1);
        subtractor.moveToThread(thread2);
        multiplier.moveToThread(thread3);
        divider.moveToThread(thread4);
        thread1.start();
        thread2.start();
        thread3.start();
        thread4.start();

        promise = EPromise(&adder, &Adder::add);
        promise.then(&multiplier, &Multiplier::multiply)
        .then(&adder, &Adder::add)
        .then(&multiplier, &Multiplier::multiply);

        timer.addTask(0, [&]{
            promise.execute(1);
        }, std::chrono::milliseconds(1000), 1);

        timer.start();
    }

    void handleException(std::exception_ptr prt)
    {
        std::cout<<"exception!!"<<std::endl;
    }
};

int main()
{
    EThread mainThread;
    Main main;
    main.moveToThread(mainThread);
    mainThread.start(false);
}
