#ifndef myCONNMANAGE_H
#define myCONNMANAGE_H

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <algorithm>

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

    void startFromTensorflow();

    void addFPGA(const char* ip, const uint port, bool bindSelf=false);
    void start();

    Worker* createWorker(Module mod, size_t numberOfJobs = 1);
    Worker* getWorker(size_t i) const {return &(*workers.at(i));}
    size_t getWorkerCount() const {return workers.size();}
    void removeFinishedWorkers();

    void setSendDelay(microseconds us) {sendDelay = us;}

    bool isRunning() const {return running;}

  private:
    std::vector<std::unique_ptr<commFPGA>> fpgas;
    std::vector<std::unique_ptr<Worker>> workers;
    
    void sendThread();
    std::future<void> sendResult;

    bool running = false;
    microseconds sendDelay = microseconds(50);
};

#endif