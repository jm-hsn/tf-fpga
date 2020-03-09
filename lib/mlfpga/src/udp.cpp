
#include "../include/udp.hpp"

int resolvehelper(const char* hostname, int family, const char* service, sockaddr_storage* pAddr)
{
    int result;
    addrinfo* result_list = NULL;
    addrinfo hints = {};
    hints.ai_family = family;
    hints.ai_socktype = SOCK_DGRAM; // without this flag, getaddrinfo will return 3x the number of addresses (one for each socket type).
    result = getaddrinfo(hostname, service, &hints, &result_list);
    if (result == 0)
    {
        //ASSERT(result_list->ai_addrlen <= sizeof(sockaddr_in));
        memcpy(pAddr, result_list->ai_addr, result_list->ai_addrlen);
        freeaddrinfo(result_list);
    }

    return result;
}

void _jobSuccess(commFPGA *fpga, jobResponse *res) {
  //printf("job %08X successful\n", *res->jobId);
  fpga->successCounter++;
  res->received = Clock::now();

  microseconds us = std::chrono::duration_cast<microseconds>(res->received - res->created);

  fpga->latency = us.count();
}
void _jobFailed(commFPGA *fpga, jobResponse *res) {
  //printf("job %08X failed %d\n", *res->jobId, res->state);
  fpga->failedCounter++;
}

// commFPGA class members


void commFPGA::recvUDP() {
  int result = 0;

  uint8_t buf[UDP_MTU];

  uint slen = sizeof(addrDest);
  result = recvfrom(sock, buf, UDP_MTU, 0, (sockaddr*)&addrDest, &slen);
  if(result == -1)
    return;

  parseRaw(buf, result);
  
}

int commFPGA::parseRaw(uint8_t *buf, uint bufLen) {
  
  jobLock.lock();
  jobResponse *currentJobResp = jobQueue[currentJobIndex];

  //check if job got deleted mid transmission

  if(currentJobResp == NULL && (recvState != checkPreamble || recvState != checkModuleId)) {
    recvState = checkPreamble;
  }
  /*if(currentJobResp != NULL) {
    printf("%15s: %4d %08d\n", ip, bufLen, currentJobResp->recvWordCounter);
  }*/

  for(int_least32_t i=0; i < (int_least32_t)bufLen/4; i++) {
    uint32_t word = __builtin_bswap32(((uint32_t*)buf)[i]);

    //printf("%15s: %4d %08X\n", ip, i, word);
    
    switch(recvState) {
      case checkPreamble:
        if(word == PREAMBLE) {
          recvState = checkJobId;
        }
        break;

      case checkJobId: 
        for(uint_least32_t k=0; k<JOB_COUNT; k++) {
          uint_least32_t kWrapped = (currentJobIndex + k) % JOB_COUNT;
          if(jobQueueJobIds[kWrapped] != word)
            continue;

          currentJobResp = jobQueue[kWrapped];
          if(currentJobResp == NULL)
            continue;

          if(*currentJobResp->jobId == word && currentJobResp->state == waiting) {
            recvState = checkModuleId;
            currentJobIndex = kWrapped;
            break;
          }
        }
        if(recvState == checkModuleId) {
          break;
        } else {
          i = i - 1 < 0 ? -1 : i - 1;
          recvState = checkPreamble;
          #ifdef DEBUG_JOB_RESP
            printf("job %08X jobId not found\n", word);
          #endif
        }
        break;
      
      case checkModuleId:
        if(*currentJobResp->moduleId == word) {
          recvState = writePayload;
          currentJobResp->recvWordCounter = 3;
          currentJobResp->state = receiving;
        } else {
          i = i - 2 < 0 ? -1 : i - 2;
          recvState = checkPreamble;
          #ifdef DEBUG_JOB_RESP
            printf("job %08X wrong moduleId %08X\n", *currentJobResp->jobId, word);
          #endif
        }
        break;
      case writePayload:
        currentJobResp->words[currentJobResp->recvWordCounter++] = word;
        if(currentJobResp->recvWordCounter >= currentJobResp->wordCount - 1) {
          recvState = checkCRC;
        }
        break;
      case checkCRC:
        *currentJobResp->crc = word;

        if(currentJobResp->checkCRC()) {
          //success
          currentJobResp->state = finished;
          jobSuccessCb(this, currentJobResp);
          jobsActive--;
          delete currentJobResp;
          jobQueue[currentJobIndex] = NULL;
          currentJobResp = NULL;

          recvState = checkPreamble;

        } else {
          //crc fail
          #ifdef DEBUG_JOB_RESP
            printf("job %08X wrong crc %08X, %4d, %4d\n", *currentJobResp->jobId, word, bufLen, i);
            for(uint_least32_t k=0; k<currentJobResp->wordCount; k++) {
              printf(" %4d %08X", k, currentJobResp->words[k]);
            }
            cout << endl;
          #endif
          jobFailedCb(this, currentJobResp);
          currentJobResp->state = waiting;
          i = i - currentJobResp->wordCount + 1 < 0 ? -1 : i - currentJobResp->wordCount + 1;
          recvState = checkPreamble;
        }
        
        break;
    }
  }

  jobLock.unlock();

  return 0;
}

commFPGA::commFPGA(const char *host, uint _port, bool bindSelf) {
  port = _port;
  strcpy(ip, host);

  jobQueue =   (jobResponse**)malloc(JOB_COUNT * sizeof(jobResponse*));
  jobQueueJobIds = (uint32_t*)malloc(JOB_COUNT * sizeof(uint32_t));

  memset(jobQueue,       0, JOB_COUNT * sizeof(jobQueue[0]));
  memset(jobQueueJobIds, 0, JOB_COUNT * sizeof(jobQueueJobIds[0]));

  jobSuccessCb = _jobSuccess;
  jobFailedCb = _jobFailed;

  int err = 0;

  struct addrinfo hints, *res;
  
  //UDP host
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;  // use IPv4
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

  getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &res);
  sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if(bindSelf)
    err = bind(sock, res->ai_addr, res->ai_addrlen);
  
  if(err != 0) {
    printf("%15s sock: %2d, err: %2d, port: %5d\n", ip, sock, err, port);
    exit(1);
  }

  //set recieve timeout
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

  //set recv buffer size

  int rcvbufsize = MAX_JOB_LEN * 4 * 2;
  setsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char*)&rcvbufsize,sizeof(rcvbufsize));
  
  //UDP client
  resolvehelper(host, AF_INET, std::to_string(port).c_str(), &addrDest);

  //send a packet to fpga to update its response port

  sendRaw((uint8_t*)"0000", 4);

}
commFPGA::~commFPGA() {
  //printf("%15s deleting job queue...\n", ip);
  
  for(uint_least32_t i=0; i<JOB_COUNT; i++) {
    if(jobQueue[i] != NULL) {
      jobFailedCb(this, jobQueue[i]);
      jobsActive--;
      delete jobQueue[i];
      jobQueue[i] = NULL;
    }
  }
}

int commFPGA::sendRaw(uint8_t *buf, uint bufLen) {
  int result = 0;

  uint_least32_t byteIndex = 0;

  while(byteIndex < bufLen) {
    uint payloadLen = bufLen - byteIndex;
    if(payloadLen > UDP_LEN/4*4)
      payloadLen = UDP_LEN/4*4;

    //printf("sending %d bytes at offset %d\n", payloadLen, byteIndex);

    result = sendto(sock, &buf[byteIndex], payloadLen, 0, (sockaddr*)&addrDest, sizeof(addrDest));
    if(result == -1) {
      int err = errno;
      std::cout << "error sending packet " << err << std::endl;
      break;
    } 
    //usleep(50);
    //printf("%d bytes sent\n", result);
    //mark n * 4 bytes as sent
    byteIndex += payloadLen;
  }
  return byteIndex;
}

int commFPGA::queueJob(jobResponse *job) {
  jobLock.lock();
  if(jobCount() >= JOB_COUNT-1)
    return -1;
  
  do {
    jobQueueIndex = (jobQueueIndex + 1) % JOB_COUNT;
  } while(jobQueue[jobQueueIndex] != NULL);
  
 /*
  jobQueueIndex = (jobQueueIndex + 1) % JOB_COUNT;
  if(jobQueue[jobQueueIndex] != NULL) {
    #ifdef DEBUG_JOB_RESP
      printf("job %08X jobQueue full, %6d active\n", *jobQueue[jobQueueIndex]->jobId, jobsActive);
    #endif
    jobFailedCb(this, jobQueue[jobQueueIndex]);
    jobsActive--;
    delete jobQueue[jobQueueIndex];
  }
  */
  jobQueue[jobQueueIndex] = job;
  jobQueueJobIds[jobQueueIndex] = *job->jobId;
  jobsActive++;
  jobLock.unlock();
  return 0;
}

int commFPGA::sendFromBuffer() {
  uint_least32_t avail = (sendBufferWriteIndex - sendBufferReadIndex) % MAX_JOB_LEN;
  
  if(avail <= 0)
    return -1;
    
  uint_least32_t readEnd;

  if(avail*4 > UDP_LEN)
    readEnd = sendBufferReadIndex + UDP_LEN / 4;
  else
    readEnd = sendBufferReadIndex + avail;

  if(readEnd >= MAX_JOB_LEN)
    readEnd = MAX_JOB_LEN;

  //printf("avail %5d read %5d write %5d len %5d\n", avail, sendBufferReadIndex, sendBufferWriteIndex, (readEnd - sendBufferReadIndex));

  int rc = sendRaw((uint8_t*)&sendBuffer[sendBufferReadIndex], (readEnd - sendBufferReadIndex) * 4);

  
  if(readEnd < MAX_JOB_LEN)
    sendBufferReadIndex = readEnd;
  else
    sendBufferReadIndex = 0;

  return rc;
}

int commFPGA::bufferJob(jobData *job, uint recvPayloadLength) {
  uint_least32_t free = (sendBufferReadIndex - sendBufferWriteIndex) % MAX_JOB_LEN;
  //printf("free %8d %8d %8d\n", free, sendBufferReadIndex, sendBufferWriteIndex);
  if(free < job->wordCount && free != 0)
    return -1;
  
  //*job->jobId = distribution(mersenne_generator);
  *job->jobId = jobIdCounter++;
  job->calcCRC();

  jobResponse *response = new jobResponse(recvPayloadLength);
  *response->moduleId = *job->moduleId;
  *response->jobId = *job->jobId;
  response->tag = job->tag;

  if(queueJob(response) == -1) {
    delete response;
    return -1;
  }

  for(uint_least32_t i=0; i<job->wordCount; i++) {
    sendBuffer[(sendBufferWriteIndex + i) % MAX_JOB_LEN] = __builtin_bswap32(job->words[i]);
  }
  sendBufferWriteIndex = (sendBufferWriteIndex + job->wordCount) % MAX_JOB_LEN;
  return job->wordCount;
}

int commFPGA::deleteOldJobs(int64_t micros) {
  
  Clock::time_point now = Clock::now();
  uint_least32_t count = 0;

  jobLock.lock();

  for(uint_least32_t i=0; i<JOB_COUNT; i++) {
    jobResponse *res = jobQueue[i];
    if(res == NULL)
      continue;
    microseconds us = std::chrono::duration_cast<microseconds>(now - res->created);
    if(us.count() > micros || us.count() < 0) {
      #ifdef DEBUG_JOB_RESP
        //printf("job %08X timed out.\n", *res->jobId);
      #endif
      jobFailedCb(this, res);
      jobsActive--;
      delete res;
      jobQueue[i] = NULL;
      count++;
    }
  }
  jobLock.unlock();
  #ifdef DEBUG_JOB_RESP
    if(count > 0) {
      printf("%4d jobs timed out\n", count);
    }
  #endif
  return count;
}

uint_least32_t commFPGA::jobCount() {
  return jobsActive;
}

void commFPGA::setSuccessCb(void (*cb) (commFPGA*, jobResponse*)) {
  jobSuccessCb = cb;
}
void commFPGA::setFailedCb(void (*cb) (commFPGA*, jobResponse*)) {
  jobFailedCb = cb;
}

std::random_device commFPGA::seed_generator;
unsigned commFPGA::seed = seed_generator();
std::uniform_int_distribution<uint32_t> commFPGA::distribution(0, UINT32_MAX);
std::mt19937 commFPGA::mersenne_generator(commFPGA::seed);