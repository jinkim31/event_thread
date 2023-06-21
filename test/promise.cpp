#include <iostream>
#include <ethread.h>
#include <etimer.h>
#include <epromise.h>

using namespace ethr;

class Adder : public EObject
{
public:
    int add(int num)
    {
        int res = num + 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout<<"+1 : "<<num<<"->"<<res<<std::endl;
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
        std::cout<<"-1 : "<<num<<"->"<<res<<std::endl;
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
        std::cout<<"*2 : "<<num<<"->"<<res<<std::endl;
        return res;
    }
};

class Divider : public EObject
{
public:
    int divide(int num)
    {
        /* Calling divide() will throw exception caught by EPromise::cat(). */
        throw std::runtime_error("divide() not implemented");
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

    /*
     * EPromise requires 2 template arguments for function argument type and the return type(promise type).
     */
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
        /*
         * EPromise supports then chaining.
         * Provide pointer to an EObject and function pointer to one of the EObject's functions.
         * The function will run in the EObject's thread.
         */
        promise
        .then(&adder, &Adder::add)
        .then(&multiplier, &Multiplier::multiply)
        .then(&subtractor, &Subtractor::subtract)
        /*
         * When using lambda for EPromise::then(), specify the return value(<int>) of the lambda.
         */
        .then<int>(this, [](int num){
            int res = num + 10;
            std::cout<<"+10 : "<<num<<"->"<<res<<std::endl;
            return res;
        })
        /*
         * Calling Divider::divide() will throw exception. EPromise::cat() is here to catch the exception.
         */
        .then(&divider, &Divider::divide)
        .cat(this, &Main::handleException);
        /*
         * EPromise::cat() can be used with a lambda like the following.
         *
         * .cat(this, [](std::exception_ptr eptr){
         *     exception handling here
         * });
         *
         * */

        /*
         * ETimer is used to execute the promise once(note that timeToLive is 1).
         */
        timer.addTask(0, [&]{
            promise.execute(1);
        }, std::chrono::milliseconds(1000), 1);
        timer.start();
    }

    void handleException(std::exception_ptr exceptionPtr)
    {
        std::cout<<"exception caught(this is an expected result): ";
        try{if(exceptionPtr) std::rethrow_exception(exceptionPtr);}
        catch(const std::runtime_error& e){std::cout<<e.what()<<std::endl;}
    }
};

int main()
{
    EThread mainThread;
    Main main;
    main.moveToThread(mainThread);
    mainThread.start(false);
}
