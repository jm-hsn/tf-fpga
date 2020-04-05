#include "worker.hpp"

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
    auto currentJobList = getJobList();
    while(running) {
      size_t remainingJobs = currentJobList->getJobCount();
      Clock::time_point now = Clock::now(); 
      commFPGA *fpga;
      
      for(size_t i=0; i<currentJobList->getJobCount(); i++) {
        {
          auto job = currentJobList->getJob(i);
          switch(job->getState()) {
            case JobState::initialized:
              throw("worker can't send job that is not ready");
              break;
            case JobState::ready:
              fpga = findAvailableFPGA();
              if(fpga == NULL) {
                goto fullQueue;
                break;
              }
              if(fpga->assignJob(job) >= 0) {
                job->setSent();
                //printf("job %08X: assigned\n", job->getJobId());
              }
              break;
            case JobState::sent:
              if(now - job->getSent() > jobTimeout) {
                fpga = (commFPGA*)job->getAssignedFPGA();
                if(fpga != NULL) {
                  if(fpga->unassignJob(job) < 0)
                    break;
                  //printf("job %08X: unassigned\n", job->getJobId());
                }
                if(job->getSendCounter() < retryCount) {
                  job->setState(JobState::ready);
                  fpga = findAvailableFPGA();
                  if(fpga == NULL) {
                    goto fullQueue;
                    break;
                  }
                  if(fpga->assignJob(job) >= 0) {
                    job->setSent();
                    //printf("job %08X: reassigned\n", job->getJobId());
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
    }
  }
  
  if(doneCb != NULL)
    doneCb();
  return 0;
}

commFPGA* Worker::findAvailableFPGA() {
  uint_least32_t minCnt = JOB_COUNT-1;
  commFPGA *fpga = NULL;
  for(std::vector<std::unique_ptr<commFPGA>>::iterator it=fpgaVector->begin(); it!=fpgaVector->end(); it++) {
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
  jobList.second->waitAll();
}