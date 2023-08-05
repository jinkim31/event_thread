#include <iostream>
#include <ethread.h>
#include <etimer.h>

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
