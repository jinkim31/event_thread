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
         * ETimer is set to execute Main::timerCallback() 5 times in 1 Hz
         */
        timer.addTask(0, [&]{
            timerCallback();
        }, std::chrono::milliseconds(1000), 5);
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
    mainThread.start(false);
}
