#include "../include/connectionManager.hpp"


const uint fpgaCount = 4;
commFPGA fpgas[fpgaCount] = {
  /*
  {"localhost", 1234},
  {"localhost", 1234},
  {"localhost", 1234},
  {"localhost", 1234},
  {"localhost", 1234},
  */
  {"192.168.88.32", 1234},
  {"192.168.88.33", 1234},
  {"192.168.88.34", 1234},
  {"192.168.88.35", 1234},
  //{"192.168.88.36", 1234},

  //{"192.168.88.37", 1234},
  //{"192.168.88.38", 1234},
  //{"192.168.88.39", 1234},
  //{"192.168.88.40", 1234},
  //{"192.168.88.41", 1234},
};

int findAvailableFPGA() {
  uint_least32_t minCnt = JOB_COUNT-1;
  uint_least32_t index = -1;
  for(uint_least32_t i=0; i<fpgaCount; i++) {
    uint_least32_t cnt = fpgas[i].jobCount();
    if(cnt < minCnt) {
      minCnt = cnt;
      index = i;
    }
  }
  return index;
}

int sendJob(jobData *job, uint recvPayloadLength) {

  //get FPGA with least pending responses
  int id = findAvailableFPGA();
  if(id < 0) {
    return -1;
  }
  return fpgas[id].bufferJob(job, recvPayloadLength);

}


uint_least32_t currentSendFPGA = 0;
bool running = true;
void *sendThread(void *ref) {
  while(running) {
    Clock::time_point start = Clock::now();
    for(uint_least32_t i=0; i<fpgaCount; i++) {
      fpgas[i].sendFromBuffer();
    }
    //printf("%8d %8d\n", fpgas[0].sendBufferWriteIndex, fpgas[0].sendBufferReadIndex);
    uint us = std::chrono::duration_cast<microseconds>(Clock::now() - start).count();
    if(us < 50)
      usleep(50 - us);
  }
  pthread_exit(NULL);
}

void *recvThread(void *ref) {
  commFPGA *fpga = (commFPGA*)ref;
  while(running) {
    fpga->recvUDP();
  }
  pthread_exit(NULL);
}

void *respThread(void *ref) {
  const uint_least32_t timeout = 1000*1000;
  while(running) {
    usleep(timeout/2);
    for(uint_least32_t i=0; i<fpgaCount; i++) {
      fpgas[i].deleteOldJobs(timeout);
    }
  }
  pthread_exit(NULL);
}

void jobSuccess(commFPGA *fpga, jobResponse *res) {
  fpga->successCounter++;
  res->received = Clock::now();

  microseconds us = std::chrono::duration_cast<microseconds>(res->received - res->created);

  fpga->latency = us.count();
}
void jobFailed(commFPGA *fpga, jobResponse *res) {
  fpga->failedCounter++;
  //printf("%08X\n", *res->jobId);
}


pthread_t tSend, tRecv[fpgaCount], tResp;
pthread_attr_t attr;

void connection_init() {

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for(uint i=0; i<fpgaCount; i++) {
    fpgas[i].setSuccessCb(jobSuccess);
    fpgas[i].setFailedCb(jobFailed);

    pthread_create(&tRecv[i], &attr, recvThread, (void*)&fpgas[i]);
    pthread_setname_np(tRecv[i], fpgas[i].ip);
  }

  pthread_create(&tSend, &attr, sendThread, 0);
  pthread_setname_np(tSend, "tSend");
  
  pthread_create(&tResp, &attr, respThread, 0);
  pthread_setname_np(tResp, "tResp");
}

void connection_close() {
  running = false;
  void *status;
  pthread_join(tSend, &status);
  for(uint i=0; i<fpgaCount; i++) {
    pthread_join(tRecv[i], &status);
  }
  pthread_join(tResp, &status);
  for(uint i=0; i<fpgaCount; i++)
    delete &fpgas[i];
}