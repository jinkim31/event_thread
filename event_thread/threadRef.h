#ifndef THREAD_REF_H
#define THREAF_REF_H

#include "eventThread.h"

template <typename EthreadType>
class ThreadRef
{
public:
    ThreadRef(){}
    ThreadRef(EthreadType* ref){mRef = ref;}
private:
    EthreadType* mRef;
friend ethr::EventThread;
};

#endif