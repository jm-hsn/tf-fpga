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
    retransmit job

  send thread:
    send 1 packet per fpga if available

  recv thread:
    recv data into response
    cb on success
    

*/

class ConnectionManager {
  public:
    ConnectionManager();
    ~ConnectionManager();

    void addFPGA(const char* ip, const uint port);

    void start();

    //send many Jobs and wait for all responses
    int sendJobListSync(std::shared_ptr<JobList> &jobList);

    //send many Jobs and call back
    int sendJobListAsync(std::shared_ptr<JobList> &jobList);

  private:
    std::vector<std::unique_ptr<commFPGA>> fpgas;
    std::vector<std::unique_ptr<Worker>> workers;
    
    void sendThread();
    std::future<void> sendResult;

    bool running = true;
};

#endif