#include <iostream>
#include "../event_thread/eventThread.h"
#include <unistd.h>
#include "firstThread.h"
#include "secondThread.h"

int main()
{
    FirstThread fThread;
    SecondThread sThread;
    fThread.start();
    sThread.start();
    sleep(3);
    fThread.stop();
    sThread.stop();
}
