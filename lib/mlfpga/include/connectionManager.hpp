#ifndef myCONNMANAGE_H
#define myCONNMANAGE_H

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

#include "commFPGA.hpp"
#include "worker.hpp"

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

class ConnectionManager {
  public:
    ConnectionManager();
    ~ConnectionManager();

    void addFPGA(const char* ip, const uint port);

    void start();

    //send many Jobs and wait for all responses
    int sendJobListSync(JobList &jobList);

    //send many Jobs and call back
    int sendJobListAsync(JobList &jobList);

  private:
    std::vector<std::reference_wrapper<commFPGA>> fpgas;
    std::vector<std::reference_wrapper<Worker>> workers;
    
    void sendThread();
    std::future<void> sendResult;

    bool running = true;
};

#endif