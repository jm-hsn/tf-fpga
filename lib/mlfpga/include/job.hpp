#ifndef myJOB_H
#define myJOB_H

#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <functional>
#include <cstring>
#include <mutex>
#include <condition_variable>

#include <chrono>

#include "modules.hpp"
#include "rng.hpp"

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds milliseconds;
typedef std::chrono::microseconds microseconds;

typedef std::function<void()> DoneCallback;

#define PREAMBLE (0xE1E4C312)

#define MAX_JOB_LEN (256*256)

enum class JobState {
  initialized,  //Job was created
  ready,        //Job is ready to be sent
  sent,   	    //sendBuf has been copied to sendQueue
  receiving,    //first part of response has been received
  finished,     //Job has been successfully received
  failed,       //Job failed too many times, throw exception
};

//generic uint32 storage
class WordBuffer {
  public:
    WordBuffer(size_t length);
    ~WordBuffer();

    uint8_t* getByteAddr() const {return bytes;}
    uint32_t* getWordAddr() const {return words;}

    uint8_t getByte(size_t i) const {return bytes[i];}
    uint8_t getWord(size_t i) const {return words[i];}

    size_t getWordCount() const {return wordCount;}
    size_t getByteCount() const {return wordCount*4;}

  protected:
      size_t wordCount;
      union {
        uint8_t *bytes;
        uint32_t *words;
      };
};

//data structure that is sent over the network
class JobData : public WordBuffer {
  public:
    JobData(uint payloadLength);

    uint32_t getPreamble() const {return words[0];}
    void setPreamble(uint32_t v) const {words[0] = v;}

    uint32_t getJobId() const {return words[1];}
    void setJobId(uint32_t v) const {words[1] = v;}

    uint32_t getModuleId() const {return words[2];}
    void setModuleId(uint32_t v) const {words[2] = v;}

    uint32_t getPayload(size_t i) const {return words[i+3];}
    void setPayload(size_t i, uint32_t v) const {words[i+3] = v;}

    uint32_t getCRC() const {return words[wordCount-1];}
    void setCRC(uint32_t v) const {words[wordCount-1] = v;}

};

//entity to track a single Job
class Job : public JobData {
  public:
    Job(Module mod);

    uint32_t tag = 0;

    //locks state
    std::mutex stateMutex;
    //locks sendBuf
    std::mutex sendMutex;
    //locks recvBuf, recvWordIndex
    std::mutex recvMutex;

    uint32_t getResponsePayload(size_t i) const {return recvBuf.getWord(i);}
    void setResponsePayload(size_t i, uint32_t v) const {recvBuf.getWordAddr()[i] = v;}
    uint32_t* getResponseAddr() const {return recvBuf.getWordAddr();}
    size_t getResponseBufferWordCount() const {return recvBuf.getWordCount();}

    void calcCRC();
    bool checkCRC();

    JobState getState() const {return state;}
    void setState(JobState s) {state = s;}

    void setReady();
    void setSent();
    void setReceived(const bool success);
    void setDoneCallback(DoneCallback cb);

    Clock::time_point getSent() const {return sent;}
    Clock::time_point getReceived() const {return received;}

    void* getAssignedFPGA() const {return assignedFPGA;}
    void setAssignedFPGA(void *fpga) {assignedFPGA = fpga;}

    size_t getSendCounter() const {return sendCounter;}

  private:
    //only payload and CRC of response
    WordBuffer recvBuf;
    DoneCallback doneCb = NULL;

    JobState state = JobState::initialized;
    Clock::time_point sent;
    Clock::time_point received;

    void *assignedFPGA = NULL;
    size_t sendCounter = 0;

};

#endif