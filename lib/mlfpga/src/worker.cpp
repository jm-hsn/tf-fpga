#include "worker.hpp"

Worker::Worker(std::vector<std::reference_wrapper<commFPGA>> &fpgas) {
  fpgaVector = &fpgas;
}
Worker::~Worker() {
  hasJobList.notify_all();
}

void Worker::start() {
  result = std::async(std::launch::async, &Worker::threadMain, this);
}

int Worker::assignJobList(JobList &jobList) {
  std::lock_guard<std::mutex> lk(currentJobList_m);
  if(currentJobList != NULL)
    return -1;
  
  currentJobList = &jobList;
  hasJobList.notify_all();

  return 0;
}

int Worker::threadMain() {
  if(currentJobList == NULL)
    return -1;

  while(currentJobList->getPendingJobCount() > 0) {
    Job *job = currentJobList->getNextJob();
    if(job == NULL) {
      break;
    }
    commFPGA *fpga = findAvailableFPGA();
    if(fpga == NULL) {
      continue;
    }
    fpga->assignJob(job);
  }
  return 0;
}

commFPGA* Worker::findAvailableFPGA() {
  uint_least32_t minCnt = JOB_COUNT-1;
  commFPGA *fpga = NULL;
  for(std::vector<std::reference_wrapper<commFPGA>>::iterator it=fpgaVector->begin(); it!=fpgaVector->end(); it++) {
    uint_least32_t cnt = it->get().jobCount();
    if(cnt < minCnt) {
      minCnt = cnt;
      fpga = &(it->get());
    }
  }
  return fpga;
}