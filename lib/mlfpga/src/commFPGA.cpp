
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
  pthread_setname_np(pthread_self(), "mlfpga recv");
  while(running) {
    int result = 0;

    uint32_t buf[UDP_MTU/4];

    uint slen = sizeof(addrDest);
    result = recvfrom(sock, (uint8_t*)buf, UDP_MTU/4, 0, (sockaddr*)&addrDest, &slen);
    if(result == -1)
      continue;

    result /= 4;

    for(int_least32_t i=0; i < result; i++) {
      buf[i] = __builtin_bswap32(buf[i]);
    }

  #ifdef DEBUG_JOB_RESP
  printf("recv ");
  for(int_least32_t i=0; i<result; i++) 
    printf("%u: %08X    ", i, buf[i]);
  printf(" %d\n", (int)recvState);
  #endif

    parseRaw(buf, result);
  }
}

int commFPGA::parseRaw(uint32_t *buf, size_t bufLen) {
  
  std::unordered_map<uint32_t,std::shared_ptr<Job>>::iterator jobIt;
  JobContainer *jobLocked = NULL;

  std::lock_guard<std::mutex> lk(jobLock);

  if(currentJob != NULL)
    jobLocked = new JobContainer(currentJob);

  for(int_least32_t i=0; i < bufLen; i++) {
    switch(recvState) {
      case RecvState::checkPreamble:
        if(buf[i] == PREAMBLE) {
          recvState = RecvState::checkJobId;
        }
        #ifdef DEBUG_JOB_RESP
          else printf("wrong preamble %08X\n", buf[i]);
        #endif
        break;

      case RecvState::checkJobId: 
        jobIt = jobList.find(buf[i]);
        if(jobIt == jobList.end()) {
          #ifdef DEBUG_JOB_RESP
            printf("job %08X jobId not found, %u\n", buf[i], i);
          #endif
          i -= 1;
          recvState = RecvState::checkPreamble;
        } else {
          currentJob = jobIt->second;
          //delete old lock
          if(jobLocked) delete jobLocked;
          //aquire lock
          jobLocked = new JobContainer(currentJob);
          if((*jobLocked)->getState() != JobState::sent) {
            #ifdef DEBUG_JOB_RESP
              printf("job %08X wasn't sent\n", buf[i]);
            #endif
            i -= 1;
            recvState = RecvState::checkPreamble;
          } else {
          assert((*jobLocked)->getAssignedFPGA() == this);
          recvState = RecvState::checkModuleId;
          }
        }
        break;
      
      case RecvState::checkModuleId:
        if((*jobLocked)->getModuleId() == buf[i]) {
          recvState = RecvState::writePayload;
          recvPayloadIndex = 0;
          (*jobLocked)->setState(JobState::sent);
        } else {
          #ifdef DEBUG_JOB_RESP
            printf("job %08X wrong moduleId %08X\n", (*jobLocked)->getJobId(), buf[i]);
          #endif
          i = i - 2 < 0 ? -1 : i - 2;
          recvState = RecvState::checkPreamble;
        }
        break;
      case RecvState::writePayload:
        (*jobLocked)->setResponsePayload(recvPayloadIndex++, buf[i]);
        if(recvPayloadIndex >= (*jobLocked)->getResponseBufferWordCount()) {
          if((*jobLocked)->checkCRC()) {
            (*jobLocked)->setReceived(true);
            #ifdef DEBUG_JOB_RESP
              printf("job %08X: success!\n", (*jobLocked)->getJobId());
            #endif
            jobList.erase((*jobLocked)->getJobId());
          } else {
            (*jobLocked)->setState(JobState::sent);
            #ifdef DEBUG_JOB_RESP
              printf("job %08X wrong crc %08X, %4lu, %4u\n", (*jobLocked)->getJobId(), buf[i], bufLen, i);
              for(size_t k=0; k<(*jobLocked)->getWordCount(); k++) {
                printf(" %4lu %08X", k, (*jobLocked)->getWord(k));
              }
              std::cout << std::endl;
            #endif
          }
          recvState = RecvState::checkPreamble;
        }
        break;
    }
  }

  if(jobLocked) delete jobLocked;

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

  //int rcvbufsize = MAX_JOB_LEN * 4 * 2;
  //setsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char*)&rcvbufsize,sizeof(rcvbufsize));
  
  //UDP client
  resolvehelper(host, AF_INET, std::to_string(port).c_str(), &addrDest);

  //send a packet to fpga to update its response port

  sendRaw((uint8_t*)"0000", 4);

}
commFPGA::~commFPGA() {
  //printf("%15s deleting job queue...\n", ip);
  running = false;
}

int commFPGA::sendRaw(uint8_t *buf, uint bufLen) {
  int result = 0;

  uint_least32_t byteIndex = 0;

  while(byteIndex < bufLen) {
    uint payloadLen = bufLen - byteIndex;
    if(payloadLen > UDP_LEN/4*4)
      payloadLen = UDP_LEN/4*4;

    result = sendto(sock, &buf[byteIndex], payloadLen, 0, (sockaddr*)&addrDest, sizeof(addrDest));
    if(result == -1) {
      int err = errno;
      std::cout << "error sending packet " << err << std::endl;
      break;
    } 
    byteIndex += payloadLen;
  }
  return byteIndex;
}

int commFPGA::assignJob(JobContainer &job) {

  if(job->getAssignedFPGA() != NULL)
    return -1;

  std::unique_lock<std::mutex> lk(jobLock, std::try_to_lock);
  if(!lk.owns_lock())
    return -1;
  
  if(jobList.size() >= JOB_COUNT)
    return -1;

  std::lock_guard<std::mutex> slk(sendLock);
  
  uint_least32_t free = MAX_JOB_LEN - sendBufferAvailable;
  if(free < job->getWordCount())
    return -1;

  jobList.insert(std::pair<uint32_t,std::shared_ptr<Job>>(job->getJobId(), job.sharedPtr()));
  job->setAssignedFPGA(this);

  uint_least32_t sendBufferWriteIndex = sendBufferReadIndex + sendBufferAvailable + 1;

  for(uint_least32_t i=0; i<job->getWordCount(); i++) {
    sendBuffer[(sendBufferWriteIndex + i) % MAX_JOB_LEN] = __builtin_bswap32(job->getWord(i));
  }
  #ifdef DEBUG_JOB_SEND
  printf("fill ");
  for(uint_least32_t i=0; i<job->getWordCount(); i++) 
    printf("%u: %08X    ", (sendBufferWriteIndex + i) % MAX_JOB_LEN, job->getWord(i));
  printf("\n");
  #endif
  sendBufferAvailable += job->getWordCount();

  return 0;
}
int commFPGA::unassignJob(JobContainer &job) {
  if(job->getAssignedFPGA() != this)
    return -1;

  std::unique_lock<std::mutex> lk(jobLock, std::try_to_lock);
  if(!lk.owns_lock())
    return -1;

  if(job->getState() == JobState::receiving) {
    currentJob = NULL;
    job->setState(JobState::sent);
    #ifdef DEBUG_JOB_RESP
      printf("job %08X: unassigned during recv\n", job->getJobId());
    #endif
  }
  job->setAssignedFPGA(NULL);
  return jobList.erase(job->getJobId());
}

int commFPGA::sendFromBuffer() {
  std::lock_guard<std::mutex> lk(sendLock);
  int_least32_t avail = sendBufferAvailable + sendBufferReadIndex > MAX_JOB_LEN ? MAX_JOB_LEN - sendBufferReadIndex : sendBufferAvailable;
  
  if(avail == 0)
    return -1;

  if(avail*4 > UDP_LEN)
    avail = UDP_LEN / 4;

  int rc = sendRaw((uint8_t*)&sendBuffer[sendBufferReadIndex], avail * 4);

  #ifdef DEBUG_JOB_SEND
  printf("send ");
  for(size_t i=0; i<avail; i++)
    printf("%lu: %08X    ", sendBufferReadIndex+i,  __builtin_bswap32(sendBuffer[sendBufferReadIndex+i]));
  printf("\n");
  #endif

  sendBufferReadIndex = (avail + sendBufferReadIndex) % MAX_JOB_LEN;
  sendBufferAvailable -= avail;

  return rc;
}

size_t commFPGA::jobCount() {
  return jobList.size();
}