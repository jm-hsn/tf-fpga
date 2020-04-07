#ifndef myUDP_H
#define myUDP_H

#include <iostream>
#include <stdio.h>
#include <assert.h> 
#include <mutex> 
#include <unordered_map>
#include <vector>
#include <chrono>

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <thread>
#include <future>
#include <string.h>

#include "job.hpp"
#include "jobList.hpp"
#include "modules.hpp"


#define UDP_LEN (1500-28-448) // size of sent UDP packets in bytes
#define UDP_MTU (1500) // size of recv UDP buffer in bytes
#define JOB_COUNT (1024 * 4 * 10) // max size of jobList

#define MAX_JOB_LEN (256*256) // max word count of job

//#define DEBUG_JOB_RESP
//#define DEBUG_JOB_SEND

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds milliseconds;
typedef std::chrono::microseconds microseconds;

enum class RecvState {
  checkPreamble,
  checkJobId,
  checkModuleId,
  writePayload
};

//using jobCb_t = void(*)(commFPGA *, jobResponse *);

class commFPGA {
  public:
    commFPGA(const char *host, uint _port = 1234, bool bindSelf = false);
    ~commFPGA();

    char ip[16];
    uint port;
    int sock;

    //called by worker thread
    
    int assignJob(JobContainer &job);
    int unassignJob(JobContainer &job);

    size_t jobCount();
    
    //called by send thread
    int sendRaw(uint8_t *buf, uint bufLen);
    int sendFromBuffer();

    void start();
    //called by recv thread
    void recvUDP();
    int parseRaw(uint32_t *buf, int_least32_t bufLen);
    
    std::shared_ptr<Job> currentJob;
    RecvState recvState = RecvState::checkPreamble;
    size_t recvPayloadIndex = 0;

    uint_least64_t successCounter = 0;
    uint_least64_t failedCounter = 0;
    float latency = 0;

  private:
    //tx buffer for buffered send function
    uint32_t *sendBuffer;
    int_least32_t sendBufferReadIndex = 0;
    int_least32_t sendBufferAvailable = 0;
    std::mutex sendLock;

    //list of pending responses
    std::unordered_map<uint32_t,std::shared_ptr<Job>> jobList;
    std::mutex jobLock;
    
    sockaddr_storage addrDest = {};
    
    std::future<void> recvResult;
    bool running = true;

};
#endif