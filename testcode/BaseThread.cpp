#include "BaseThread.h"

void BaseThread::basePrint()
{
    std::cout<<"base print "<<std::endl;
}

BaseThread::BaseThread(): EventThread("base")
{

}

