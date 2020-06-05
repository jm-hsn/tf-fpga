#include "jobList.hpp"

JobList::JobList(Module mod, size_t numberOfJobs) {
  jobCount = numberOfJobs;
  pendingJobCount = numberOfJobs;
  for(size_t i=0; i<numberOfJobs; i++) {
    std::shared_ptr<Job> job(new Job(mod));
    job->setDoneCallback([this]{
      finishJob();
    });
    jobs.push_back(job);
  }
}

void JobList::waitAll() {
  std::unique_lock<std::mutex> lk(pendingJobCount_m);
  jobListDone.wait(lk, [this]{return pendingJobCount <= 0;});
}

void JobList::waitOne(microseconds us) {
  std::unique_lock<std::mutex> lk(pendingJobCount_m);
  jobListDone.wait_for(lk, us);
}

void JobList::finishJob() {
  std::lock_guard<std::mutex> lk(pendingJobCount_m);
  pendingJobCount--;
  jobListDone.notify_all();
}

JobContainer JobList::getJob(size_t i) {
  return JobContainer(jobs.at(i));
}

JobContainer JobList::getNextJob() {
  nextJobIndex = (nextJobIndex+1) % jobCount;
  return JobContainer(jobs.at(nextJobIndex));
}