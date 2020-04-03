#include "worker.hpp"

Worker::Worker(std::vector<std::unique_ptr<commFPGA>> *fpgas) {
  fpgaVector = fpgas;
}
Worker::~Worker() {
  hasJobList.notify_all();
}

void Worker::startAsync() {
  result = std::async(std::launch::async, &Worker::threadMain, this);
}
void Worker::startSync() {
  threadMain();
}

int Worker::assignJobList(std::shared_ptr<JobList> &jobList) {
  std::lock_guard<std::mutex> lk(currentJobList_m);
  if(currentJobList != NULL)
    return -1;
  
  currentJobList = jobList;
  hasJobList.notify_all();

  return 0;
}

int Worker::threadMain() {
  if(currentJobList == NULL)
    return -1;

  while(true) {
    size_t remainingJobs = currentJobList->getJobCount();
    Clock::time_point now = Clock::now(); 
    commFPGA *fpga;
    
    for(size_t i=0; i<currentJobList->getJobCount(); i++) {
      std::shared_ptr<Job> &job = currentJobList->getJob(i);
      switch(job->getState()) {
        case JobState::initialized:

          break;
        case JobState::ready:
          sendJob(job);
          break;
        case JobState::sent:
          if(std::chrono::duration_cast<microseconds>(now - job->getSent()).count() > 1000) {
            fpga = (commFPGA*)job->getAssignedFPGA();
            if(fpga != NULL) {
              fpga->unassignJob(job);
            }
            if(job->getSendCounter() < 5) {
              job->setState(JobState::ready);
              sendJob(job);
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
    if(remainingJobs <= 0) {
      break;
    }
    currentJobList->waitOne(microseconds(1000));
  }
  return 0;
}

void Worker::sendJob(std::shared_ptr<Job> &job) {
    commFPGA *fpga = findAvailableFPGA();
    if(fpga == NULL) {
      return;
    }
    if(fpga->assignJob(job) >= 0) {
      job->setSent();
    }
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