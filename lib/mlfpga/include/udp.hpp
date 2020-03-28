#ifndef myUDP_H
#define myUDP_H

#include <iostream>
#include <stdio.h>
#include <assert.h> 
#include <mutex> 
#include <map>
#include <vector>
#include <random>
#include <chrono>


#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <pthread.h>
#include <string.h>

#include "job.hpp"
#include "modules.hpp"


#define UDP_LEN (1500-28-448) // size of sent UDP packets in bytes
#define UDP_MTU (1500) // size of recv UDP buffer in bytes
#define JOB_COUNT (1024 * 4)

//#define DEBUG_JOB_RESP


typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds milliseconds;
typedef std::chrono::microseconds microseconds;

typedef enum {
  checkPreamble,
  checkJobId,
  checkModuleId,
  writePayload,
  checkCRC
} recvState_t;

//using jobCb_t = void(*)(commFPGA *, jobResponse *);

class commFPGA {
  public:
    commFPGA(const char *host, uint _port = 1234, bool bindSelf = false);
    ~commFPGA();

    char ip[16];
    uint port;
    int sock;

    //called by worker thread
    
    int queueJob(jobResponse *job);
    int bufferJob(jobData *job, uint recvPayloadLength);

    uint_least32_t jobCount();
    
    //called by send thread
    int sendRaw(uint8_t *buf, uint bufLen);
    int sendFromBuffer();

    //called by recv thread
    void recvUDP();
    int parseRaw(uint8_t *buf, uint bufLen);
    
    //jobQueue[] being worked on, search for next job begins at currentJobIndex+1
    uint_least32_t currentJobIndex = 0;
    recvState_t recvState = checkPreamble;


    uint_least64_t successCounter = 0;
    uint_least64_t failedCounter = 0;
    float latency = 0;

    //called by resp thread

    int deleteOldJobs(int64_t micros=50000);
    
    void (*jobSuccessCb) (commFPGA *, jobResponse *);
    void setSuccessCb(void (*cb) (commFPGA *, jobResponse *));

    void (*jobFailedCb) (commFPGA *, jobResponse *);
    void setFailedCb(void (*cb) (commFPGA *, jobResponse *));

  protected:
    //tx buffer for buffered send function
    uint32_t sendBuffer[MAX_JOB_LEN];
    uint_least32_t sendBufferReadIndex = 0;
    uint_least32_t sendBufferWriteIndex = 0;

    //list of pending responses
    jobResponse* *jobQueue;
    //list of associated jobIds as search index
    uint32_t *jobQueueJobIds;

    uint_least32_t jobQueueIndex = 0;
    uint_least32_t jobsActive = 0;
    std::mutex jobLock;

    //listener for a single FPGA
    
    sockaddr_storage addrDest = {};
    //pthread_t tRecv;
    //pthread_attr_t attr;
    //volatile bool running = 1;

    //rng for the jobIds  
    uint32_t jobIdCounter = 0;
    static std::random_device seed_generator;
    static unsigned seed;
    static std::mt19937 mersenne_generator;
    static std::uniform_int_distribution<uint32_t> distribution;
};
#endif