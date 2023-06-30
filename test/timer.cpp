#include <iostream>
#include <ethread.h>
#include <etimer.h>

using namespace ethr;

class Main : public EObject
{
public:
    ETimer timer;

    Main()
    {
        /*
         * first task(id=0) that executes Main::timerCallback() 5 times in 1 Hz
         */
        timer.addTask(0, [&]{
            timerCallback();
        }, std::chrono::milliseconds(1000), 5);

        /*
         * second task(id=1) that removes first task(id=0) 3 seconds after the timer start()
         */
        timer.addTask(1, [&]{
            if(timer.removeTask(0))
                std::cout<<"Task removed"<<std::endl;
            }, std::chrono::milliseconds(3000), 1);

        /*
         * third task(id=2) that stops main thread and terminates the application 5 seconds after timer start()
         */
        timer.addTask(2, []{
            EThread::stopMainThread();
        }, std::chrono::milliseconds(5000), 1);

        /*
         * start timer
         */
        timer.start();
    }

    void timerCallback()
    {
        std::cout<<"timer callback"<<std::endl;
    }
};

int main()
{
    EThread mainThread;
    Main main;
    main.moveToThread(mainThread);
    mainThread.start(true);
}
