#include <iostream>
#include "../event_thread/eventThread.h"

using namespace ethr;

class Object : public EObject
{
public:
    void hi(std::string name){std::cout<<"hi "<<name<<std::endl;}
};
int main()
{
    Object o;
    EThread thread("test");
    o.moveToThread(thread);
    o.callQueued(&Object::hi, std::string("jin"));
    thread.start();
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    thread.stop();
}
