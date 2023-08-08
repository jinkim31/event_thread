# Event Thread
### An Asynchronous Multithreading Framework

# Getting Started

## The Main Thread

```c++
#include <iostream>
#include <ethread.h>

#include "app.h"

using namespace ethr;

int main()
{
    EThread mainThread;
    EThread::provideMainThread(mainThread);
    App app;
    app.moveToThread(mainThread);
    mainThread.start(); // hangs here until EThread::stopMainThread() is called
    app.removeFromThread();
}
```
Event Thread uses thread & worker architecture which means you need to define a worker class
which contains the process you want to run and move the worker to the main thread so that the worker can actually work.
The main worker class here in the example is the `App` class, and it inherits from `EObject` class.
In the main function, a new `EThread` called `mainThread` is created and set as the main thread with `EThread::provideMainThread()`.
Then, instance of `App` is created and moved to the `mainThread`.
Finally, the main thread `mainThread` is started using `EThread.start()`. 
Since it is the main thread, `EThread.start()` blocks the flow and runs the event loop that handles the events and inter-thread communications
(if it's not the main thread, `EThread.start()` won't block and create new thread to run its event loop).
When `EThread::stopMainThread()` is called anywhere in the program, event loop of the main thread is stopped and `EThread.start()` returns.
For every thread worker that moved to a thread, it has to be explicitly removed from the thread before the thread is destructed.
Therefore, `EObject::removeFromThread()` is used to remove the worker `app` from the thread before `mainThread` is go out of the scope and destructed.

## Timers
```c++
#include <ethread.h>
#include <etimer.h>
#include "worker.h"

using namespace ethr;

class App : public EObject
{
public:
    App();
    ~App();
    void progressReported(int progress);
private:
    ETimer mTimer;
};

App::App()
{
    mTimer.moveToThread(EThread::mainThread());
    mTimer.addTask(0, std::chrono::milliseconds(1000), [&]{
        std::cout<<"timer callback"<<std::endl;
    }, 3);
    mTimer.addTask(1, std::chrono::milliseconds(4000), [&]{
        EThread::stopMainThread();
    }, 1);
    mTimer.start();
}

App::~App()
{
    mTimer.removeFromThread();
}
```
`ETimer` is used to make some process run in a periodic loop. `ETimer` is a thread worker that inherits from `EObject` so
it has to be moved to a thread for it to work. In the example above the `App` class that has been used in `The Main Thread` example is shown.
In that example, an instance of `App` is created and moved to the main thread. 
In the `App` class, it has a `ETimer` `mTimer` as a member and moves it to the same main thread in the constructor.
The constructor adds a task to the timer using `ETimer::addTask()` 
which takes a unique task id, period, callback lambda that would be called every loop, and optional ttl(time to live) of the task.
The first task of id 0 prints "timer callback" is 3 times with 1000 milliseconds interval between.
The second task of id 1 calls `EThread::stopMainThread()` which stops the main thread and finally terminates the program.
`ETimer::start()` finally starts the timer execute the tasks.

> The lambda passed as `callback` parameter of `ETimer::addTask()` will run in the thread that the `ETimer` is moved to.
> For thread safety, make sure to move the `EThread` to the same thread that the parent EObject(in this case `App`) is in.

> Also note that in the destructor, `mTimer` is removed from the thread for a proper `EThread` destruction. 

## Inter-thread Communications
Event Thread support thread-safe inter-thread communications that executes public member function of the other `EObject`s of other threads.

`app.h`
```c++
#ifndef EVENT_THREAD_APP_H
#define EVENT_THREAD_APP_H

#include <ethread.h>
#include <etimer.h>
#include "worker.h"

using namespace ethr;

class App : public EObject
{
public:
    App();
    ~App();
    void progressReported(int progress);
private:
    ETimer mTimer;
    EThread mWorkerThread;
    Worker mWorker;
};


#endif
```
`app.cpp`
```c++
#include "app.h"

App::App()
{
    mWorker.moveToThread(mWorkerThread);
    mWorker.setAppRef(this->ref<App>());
    mWorkerThread.start();

    mTimer.moveToThread(EThread::mainThread());
    mTimer.addTask(0, std::chrono::milliseconds(0), [&]{
        mWorker.callQueued(&Worker::work);
    }, 1);
    mTimer.start();
}

App::~App()
{
    mWorkerThread.stop();
    mWorker.removeFromThread();
    mTimer.removeFromThread();
}

void App::progressReported(int progress)
{
    std::cout<<"progress: "<<progress<<"/99"<<std::endl;
    if(progress == 99)
        EThread::stopMainThread();
}
```
Here's an updated version of `App` class. It now has a new worker, `mWorker` and a thread `mWorkerThread` that `mWorker` is moved to.
`mWorker`is an instance of class `Worker` which will be explained later.
With `ETimer::addTask()` a new task is added to the timer that after 0 millisecond of the timer start,
it will execute a public member function `Worker::work`. This is implemented by calling `EObject::callQueued()` with the function pointer as the parameter.

`worker.h`
```c++
#ifndef EVENT_THREAD_WORKER_H
#define EVENT_THREAD_WORKER_H

#include <ethread.h>

using namespace ethr;

class App; // forward declaration

class Worker : public EObject
{
public:
    void setAppRef(const EObjectRef<App>& appRef);
    void work();
private:
    EObjectRef<App> mAppRef;
};

#endif
```
`worker.cpp`
```c++
#include "worker.h"
#include "app.h"

void Worker::setAppRef(const EObjectRef<App> &appRef)
{
    mAppRef = appRef;
}

void Worker::work()
{
    for(int i=0; i<100; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        mAppRef.callQueued(&App::progressReported, i);
    }
}
```
Here's the `Worker` class mentioned above. It's a thread worker that has the public member function `Worker::work()`, the very function that `app` called using `EObject::callQueued()`.
`Worker::work()` performs a "time-consuming" operation which is just a combination of sleep and another `EObject::callQueued()` that calls one of `app`'s public functions `App::progressReported()`
However, in this case two things are different. 
First, `callQueued()` is called with the reference of the `app`, `EObjectRef<App> mAppRef`. The reference is created and passed to the `mWorker` using the dependency injection function `void setAppRef()`.
Second, `App::progressReported` has a `int` parameter. This parameter can be passed by using the variadic argument of `callQueued()`.

> Rather than passing raw pointers to other threads which is extremely unsafe due to the dangling pointers, it is highly recommended to use reference when passing `EObjects` to other threads.

> For this kind of architecture where two classes uses each other, mutual inclusion that two header files include each other is a common issue.
> As shown in the example(in `worker.h` and `worker.cpp`), forward-declare the other class in the header file and include in the cpp file.

## Promises
`EPromise` class provides high-level async operations that resemble promises from Javascript.
`EPromise::then()` is used to chain multiple function to be executed sequentially from different objects across different threads.
`EPromise::cat()` is used to capture any exceptions that are thrown during the execution of the chain.
```c++
#include <ethread.h>
#include <etimer.h>
#include <epromise.h>

using namespace ethr;

class Worker : public EObject
{
public:
    void setMultiplier(int multiplier)
    {
        mMultiplier = multiplier;
    }

    int multiply(int n)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout<<n<<"*"<<mMultiplier<<"="<<n*mMultiplier<<std::endl;
        return n*mMultiplier;
    }
private:
    int mMultiplier;
};

class App : public EObject
{
public:
    App()
    {
        for(int i=0; i<10; i++)
        {
            workers[i].setMultiplier(i+1);
            workers[i].moveToThread(threads[i]);
            threads[i].start();
        }
        timer.moveToThread(EThread::mainThread());
        timer.addTask(0, std::chrono::milliseconds(0), [&]
        {
            auto promise = new EPromise(workers[0].ref<Worker>(), &Worker::multiply);
            promise
            ->then(workers[1].ref<Worker>(), &Worker::multiply)
            ->then(workers[2].ref<Worker>(), &Worker::multiply)
            ->then(workers[3].ref<Worker>(), &Worker::multiply)
            ->then(workers[4].ref<Worker>(), &Worker::multiply)
            ->then(workers[5].ref<Worker>(), &Worker::multiply)
            ->then(workers[6].ref<Worker>(), &Worker::multiply)
            ->then(workers[7].ref<Worker>(), &Worker::multiply)
            ->then(workers[8].ref<Worker>(), &Worker::multiply)
            ->then(workers[9].ref<Worker>(), &Worker::multiply);
            promise->execute(2);
        }, 1);
        timer.addTask(1, std::chrono::milliseconds(6000), []
        {
            EThread::stopMainThread();
        }, 1);
        timer.start();
    }
    ~App()
    {
        timer.removeFromThread();
        for(auto & worker : workers)
            worker.removeFromThread();
        for(auto & thread : threads)
            thread.stop();
    }
private:
    ETimer timer;
    Worker workers[10];
    EThread threads[10];
};

int main()
{
    EThread mainThread("main");
    EThread::provideMainThread(mainThread);
    App app;
    app.moveToThread(mainThread);
    mainThread.start();
    app.removeFromThread();
}
```
In the constructor of class `App`, 10 workers are moved to their own threads and the threads are started. 
Then, a task is added to `ETimer mTimer` that uses promises to chain function calls between different threads.

> Note that `EPromise` has to be dynamically allocated using `new`. 
> The user should NOT delete the allocated promise since the deletion is handled automatically.

JinKim2022@AnsurLab@KIST\
JinKim2023@HumanLab@KAIST