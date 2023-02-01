#include <iostream>
#include "../event_thread/eventThread.h"
#include <unistd.h>
#include "MainThread.h"

int main()
{
    MainThread mainThread;
    mainThread.start(true);
    mainThread.stop();
}
