#include "worker.hpp"

//#define DEBUG_WORKER

Worker::Worker(std::vector<std::unique_ptr<commFPGA>> *fpgas, Module mod, size_t numberOfJobs) : 
jobList(std::piecewise_construct, std::make_tuple(), std::make_tuple(new JobList(mod, numberOfJobs))) {
  fpgaVector = fpgas;
}
Worker::~Worker() {
  running = false;
}

void Worker::startAsync() {
  result = std::async(std::launch::async, &Worker::threadMain, this);
}
void Worker::startSync() {
  threadMain();
}

JobListContainer Worker::getJobList() {
  return JobListContainer(jobList);
}

int Worker::threadMain() {
  pthread_setname_np(pthread_self(), "mlfpga worker");
  {
    size_t lastI = 0;
    auto currentJobList = getJobList();
    int rc;
    while(running) {
      size_t remainingJobs = currentJobList->getJobCount();
      Clock::time_point now = Clock::now();
      #ifdef DEBUG_WORKER
        Clock::time_point then;
        size_t sentBytes = 0;
      #endif
      commFPGA *fpga;
      
      for(size_t i=0; i<currentJobList->getJobCount(); i++) {
        {
          size_t currentI = (lastI + i) % currentJobList->getJobCount();
          auto job = currentJobList->getJob(currentI);
          switch(job->getState()) {
            case JobState::initialized:
              throw("worker can't send job that is not ready");
              break;
            case JobState::ready:
              fpga = findAvailableFPGA();
              if(fpga == NULL) {
                lastI = currentI;
                goto fullQueue;
                break;
              }
              rc = fpga->assignJob(job);
              //printf("rc: %4d i: %4lu\n", rc, i);
              if(rc >= 0) {
                job->setSent();
                #ifdef DEBUG_WORKER
                  sentBytes += job->getByteCount();
                  printf("job %08X: \x1b[32massigned\x1b[0m   no.: %3lu\n", job->getJobId(), currentI);
                #endif
              } else if(rc == -4) {
                lastI = currentI;
                goto fullQueue;
              }
              break;
            case JobState::sent:
              if(now - job->getSent() > jobTimeout) {
                fpga = (commFPGA*)job->getAssignedFPGA();
                if(fpga != NULL) {
                  if(fpga->unassignJob(job) < 0)
                    break;
                  #ifdef DEBUG_WORKER
                    printf("job %08X: \x1b[31munassigned\x1b[0m no.: %3lu\n", job->getJobId(), currentI);
                  #endif
                }
                if(job->getSendCounter() < retryCount) {
                  job->setState(JobState::ready);
                  fpga = findAvailableFPGA();
                  if(fpga == NULL) {
                    lastI = currentI;
                    goto fullQueue;
                    break;
                  }
                  if(fpga->assignJob(job) >= 0) {
                    job->setSent();
                    #ifdef DEBUG_WORKER
                      sentBytes += job->getByteCount();
                      printf("job %08X: \x1b[33mreassigned\x1b[0m no.: %3lu\n", job->getJobId(), currentI);
                    #endif
                  }
                } else {
                  job->setState(JobState::failed);
                  job->setReceived(false);
                }
              }
              break;
            case JobState::receiving:

              break;
            case JobState::finished:
              remainingJobs--;
              break;
            case JobState::failed:
              remainingJobs--;
              break;
          }
        }
      }
      if(remainingJobs <= 0) {
        break;
      }
      fullQueue:
      currentJobList->waitOne(jobTimeout);
      #ifdef DEBUG_WORKER
        then = Clock::now();
        printf("loop: %3ld ms sent: %5lu kB remaining: %lu\n", std::chrono::duration_cast<milliseconds>(then - now).count(), sentBytes/1024, remainingJobs);
      #endif
    }
  }
  
  if(doneCb != NULL)
    doneCb();

  running = false;
  return 0;
}

commFPGA* Worker::findAvailableFPGA() {
  uint_least32_t minCnt = JOB_COUNT-1;
  commFPGA *fpga = NULL;
  for(auto it=fpgaVector->begin(); it!=fpgaVector->end(); it++) {
    uint_least32_t cnt = it->get()->jobCount();
    if(cnt < minCnt) {
      minCnt = cnt;
      fpga = it->get();
    }
  }
  return fpga;
}

void Worker::setDoneCallback(DoneCallback cb) {
  doneCb = cb;
}

void Worker::waitUntilDone() {
  if(!running)
    return;
  jobList.second->waitAll();
}