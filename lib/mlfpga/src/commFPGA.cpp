
#include "../include/commFPGA.hpp"

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


// commFPGA class members

void commFPGA::start() {
  recvResult = std::async(std::launch::async, &commFPGA::recvUDP, this);
}

void commFPGA::recvUDP() {
  while(running) {
    int result = 0;

    uint32_t buf[UDP_MTU/4];

    uint slen = sizeof(addrDest);
    result = recvfrom(sock, (uint8_t*)buf, UDP_MTU/4, 0, (sockaddr*)&addrDest, &slen);
    if(result == -1)
      return;

    result /= 4;

    for(int_least32_t i=0; i < result; i++) {
      buf[i] = __builtin_bswap32(buf[i]);
    }

    parseRaw(buf, result);
  }
}

int commFPGA::parseRaw(uint32_t *buf, size_t bufLen) {
  jobLock.lock();

  for(size_t i=0; i < bufLen; i++) {
    
    switch(recvState) {
      case RecvState::checkPreamble:
        if(buf[i] == PREAMBLE) {
          recvState = RecvState::checkJobId;
        }
        break;

      case RecvState::checkJobId: 
        currentJob = jobList.find(buf[i]);
        if(currentJob == jobList.end()) {
          i -= 1;
          recvState = RecvState::checkPreamble;
        } else if(currentJob->second->getState() != JobState::sent) {
          #ifdef DEBUG_JOB_RESP
            printf("job %08X wasn't sent\n", buf[i]);
          #endif
          i -= 1;
          recvState = RecvState::checkPreamble;
        } else {
          #ifdef DEBUG_JOB_RESP
            printf("job %08X jobId not found\n", buf[i]);
          #endif
          recvState = RecvState::checkModuleId;
        }
        break;
      
      case RecvState::checkModuleId:
        if(currentJob->second->getModuleId() == buf[i]) {
          recvState = RecvState::writePayload;
          recvPayloadIndex = 0;
          currentJob->second->setState(JobState::sent);
        } else {
          i = i - 2 < 0 ? -1 : i - 2;
          recvState = RecvState::checkPreamble;
          #ifdef DEBUG_JOB_RESP
            printf("job %08X wrong moduleId %08X\n", *currentJobResp->jobId, word);
          #endif
        }
        break;
      case RecvState::writePayload:
        currentJob->second->setResponsePayload(recvPayloadIndex++, buf[i]);
        if(recvPayloadIndex >= currentJob->second->getResponseBufferWordCount()) {
          if(currentJob->second->checkCRC()) {
            currentJob->second->setState(JobState::finished);
            currentJob->second->isComplete();
            jobList.erase(currentJob->second->getJobId());
          } else {
            currentJob->second->setState(JobState::sent);
            #ifdef DEBUG_JOB_RESP
              printf("job %08X wrong crc %08X, %4d, %4d\n", *currentJobResp->jobId, word, bufLen, i);
              for(uint_least32_t k=0; k<currentJobResp->wordCount; k++) {
                printf(" %4d %08X", k, currentJobResp->words[k]);
              }
              cout << endl;
            #endif
          }
          recvState = RecvState::checkPreamble;
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

int commFPGA::assignJob(Job *job) {
  jobLock.lock();
  if(jobList.size() >= JOB_COUNT)
    return -1;
  
  jobList.insert(std::pair<uint32_t,Job*>(job->getJobId(), job));
  
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

int commFPGA::fillBuffer(JobData *jobData) {
  uint_least32_t free = (sendBufferReadIndex - sendBufferWriteIndex) % MAX_JOB_LEN;
  //printf("free %8d %8d %8d\n", free, sendBufferReadIndex, sendBufferWriteIndex);
  if(free < jobData->getWordCount() && free != 0)
    return -1;

  for(uint_least32_t i=0; i<jobData->getWordCount(); i++) {
    sendBuffer[(sendBufferWriteIndex + i) % MAX_JOB_LEN] = __builtin_bswap32(jobData->getWord(i));
  }
  sendBufferWriteIndex = (sendBufferWriteIndex + jobData->getWordCount()) % MAX_JOB_LEN;
  return 0;
}

uint_least32_t commFPGA::jobCount() {
  return jobsActive;
}