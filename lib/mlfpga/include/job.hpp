#ifndef myJOB_H
#define myJOB_H

#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>

#include <chrono>

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds milliseconds;
typedef std::chrono::microseconds microseconds;

#define PREAMBLE (0xE1E4C312)

#define MAX_JOB_LEN (256*256)

typedef enum {
  waiting,
  receiving,
  finished
} jobState_t;

//data structure of a job
class jobData {
  public:
    jobData(uint payloadLength);
    ~jobData();

    union {
      uint8_t *bytes;
      uint32_t *words;
      //pointer to words[0]
      uint32_t *preamble;
    };

    uint32_t wordCount;
    Clock::time_point created = Clock::now();
    uint32_t tag = 0;

    //pointer to words[1]
    uint32_t *jobId;
    //pointer to words[2]
    uint32_t *moduleId; 
    //array at &words[3]
    uint32_t *payload;
    //pointer to words[wordCount-1]
    uint32_t *crc;

    void calcCRC();
    bool checkCRC();
};

class jobResponse : public jobData {
  public:
    using jobData::jobData;
    jobState_t state = waiting;
    uint recvWordCounter = 0;
    Clock::time_point received;
};

#endif