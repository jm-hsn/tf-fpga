#ifndef myDUMMY_H
#define myDUMMY_H

#include "connectionManager.hpp"
#include <iostream>
#include <csignal>

class dummyJob : public jobData {
  public:
    dummyJob(void);
    const uint payloadLen = 4;
};

class dummyBigJob : public jobData {
  public:
    dummyBigJob(void);
    const uint payloadLen = 1024;
};

void perfTest();
void fillTest();


#endif