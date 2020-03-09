#include "../include/dummy.hpp"


dummyJob::dummyJob(void) : jobData(4) {
  *moduleId = moduleIds[dummyModule];
  *crc = 0xAAAAAAAA;

  for(uint_least32_t i=0; i<payloadLen; i++) {
    payload[i] = i+1;
  }
}

dummyBigJob::dummyBigJob(void) : jobData(1024) {
  *moduleId = moduleIds[dummyBigModule];
  *crc = 0xAAAAAAAA;

  for(uint_least32_t i=0; i<payloadLen; i++) {
    payload[i] = i+1;
  }
}

void perfTest() {
  connection_init();
  usleep(5000);

  Clock::time_point start = Clock::now();
  uint_least64_t *cnt = (uint_least64_t*)malloc(sizeof(uint_least64_t) * fpgaCount);
  assert(cnt != NULL);

  dummyJob job;
  uint_least32_t rejected = 0, successful = 0;

  for(uint i=0; i<job.payloadLen; i++) {
    job.payload[i] = i+1;
  }
  
  while(1) {
    
    for(uint_least32_t i=0; i<MAX_JOB_LEN / job.wordCount / 2; i++) {
      int ret = sendJob(&job, job.payloadLen);
      if(ret < 0)
        rejected++;
      else
        successful++;
      
    }
    usleep(0);
    Clock::time_point now = Clock::now();

    if(chrono::duration_cast<milliseconds>(now - start).count() > 200) {
      printf("rejected: %8d, sent: %8d\n", rejected, successful);
      for(uint_least32_t k=0; k<fpgaCount; k++) {
        printf(
          "%15s  succ: %9ld fail: %9ld (%8.5f%%) %6.3f ms queue:%5d, %6ld jobs/s \n", 
          fpgas[k].ip,
          fpgas[k].successCounter, 
          fpgas[k].failedCounter, 
          100.0 * fpgas[k].failedCounter / (1+fpgas[k].successCounter + fpgas[k].failedCounter), 
          fpgas[k].latency/1000,
          fpgas[k].jobCount(),
          chrono::duration_cast<microseconds>(now - start).count() * (fpgas[k].successCounter-cnt[k]) *5 / 1000000
        );
        cnt[k] = fpgas[k].successCounter;
      }

      printf("\n");
      start = now;
    }
  }
}
void printJobResp(commFPGA *fpga, jobResponse *res) {
  printf("%s:\n", fpga->ip);
  uint_least16_t i = 0;

  for(; i<res->wordCount; i++) {

    if(i%8 == 0) {
      printf("\n %4d:", i/8);
    }
    //if(res->words[i] == 0)
    //  break;
    printf(" %08X", res->words[i]);
  }
  printf("\n");
  
}

void fillTest() {

  connection_init();
  usleep(50);

  jobData job(4);
  *job.jobId = 0x12345678;

  for(uint i=0; i<job.wordCount-4; i++) {
    job.payload[i] = i+1;
  }
  job.calcCRC();

  for(uint i=0; i<fpgaCount; i++) {

    jobResponse *response = new jobResponse(4);
    *response->moduleId = *job.moduleId;
    *response->jobId = *job.jobId;

    uint32_t buf[UDP_LEN];
    uint n = 1;

    for(uint k=0; k<n; k++) {
      for(uint_least32_t i=0; i<job.wordCount; i++) {
        buf[i+k*job.wordCount] = __builtin_bswap32(job.words[i]);
      }
    }

    //fpgas[i].setFailedCb(&printJobResp);
    fpgas[i].setSuccessCb(&printJobResp);

    fpgas[i].queueJob(response);

    fpgas[i].sendRaw((uint8_t*)buf, n * job.wordCount * 4);

  }
  usleep(100000);

  connection_close();

}