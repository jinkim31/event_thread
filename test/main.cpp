#include <iostream>
#include "../event_thread/event_thread.h"
#include "../event_thread/event_thread_util.h"

using namespace ethr;

int main()
{
    ETimer timer;
    EThread thread("test");
    timer.addTask(0, []{std::cout<<"callback"<<std::endl;}, std::chrono::milliseconds(1000));
    timer.moveToThread(thread);
    thread.start();
    timer.start();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    thread.stop();
}
