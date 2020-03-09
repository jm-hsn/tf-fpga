#ifndef myCONNMANAGE_H
#define myCONNMANAGE_H

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include "udp.hpp"

/*
  worker thread:
    takes jobs
    assigns free fpga
    queue response
    cb on overwrite + delete old resp
    fills send buffer

  send thread:
    cycle fd
    send 1 packet if available

  recv thread:
    select(readFD)
    recv data into response
    cb on success + delete response

  response thread:
    search old responses
    cb on timeout + delete response
    

*/

extern commFPGA fpgas[];
extern const uint fpgaCount;

int sendJob(jobData *job, uint recvPayloadLength);
void connection_init();
void connection_close();


#endif