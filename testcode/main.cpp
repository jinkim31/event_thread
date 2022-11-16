#include <iostream>
#include "../event_thread/eventThread.h"
#include <unistd.h>
#include "firstThread.h"
#include "secondThread.h"

int main()
{
    FirstThread fThread;
    SecondThread sThread;

    fThread.setLoopFreq(10);
    sThread.setLoopFreq(10);

    fThread.start();
    sThread.start();

    sleep(3);

    fThread.stop();
    sThread.stop();
}
