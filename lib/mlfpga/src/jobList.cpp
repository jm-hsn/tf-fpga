#include "jobList.hpp"

JobList::JobList(Module mod, size_t numberOfJobs) {
  jobCount = numberOfJobs;
  pendingJobCount = numberOfJobs;
  for(size_t i=0; i<numberOfJobs; i++) {
    //jobs.emplace_back(mod);
    Job job(mod);
    job.setDoneCallback([this]{
      finishJob();
    });
    jobs.push_back(job);
  }
}

void JobList::waitAll() {
  std::unique_lock<std::mutex> lk(pendingJobCount_m);
  jobListDone.wait(lk, [this]{return pendingJobCount <= 0;});
}

void JobList::finishJob() {
  std::lock_guard<std::mutex> lk(pendingJobCount_m);
  pendingJobCount--;
  jobListDone.notify_all();
}

Job& JobList::getJob(size_t i) {
  return jobs.at(i);
}

Job* JobList::getNextJob() {
  for(size_t i=0; i<jobCount; i++) {
    size_t rotated_i = (i+nextJobIndex+1) % jobCount;
    if(jobs.at(rotated_i).get().getState() == JobState::ready) {
      nextJobIndex = rotated_i;
      return &jobs.at(rotated_i).get();
    }
  }
  return NULL;
}

void JobList::setDoneCallback(DoneCallback cb) {
  doneCb = cb;
}