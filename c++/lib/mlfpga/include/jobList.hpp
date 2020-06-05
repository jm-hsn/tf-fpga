#ifndef myJOBLIST_H
#define myJOBLIST_H

#include <functional>
#include <vector>
#include "job.hpp"
#include "modules.hpp"

//entity to track an array of similar jobs
class JobList {
  public:
    JobList(Module mod, size_t numberOfJobs);
    void waitAll();
    void waitOne(microseconds us);
    void finishJob();

    size_t getPendingJobCount() const {return pendingJobCount;}

    JobContainer getJob(size_t i);
    JobContainer getNextJob();
    size_t getJobCount() const {return jobCount;}
    
    std::mutex jobListLock;
  private:
    std::vector<std::shared_ptr<Job>> jobs;

    size_t jobCount;
    size_t pendingJobCount;
    std::condition_variable jobListDone;
    std::mutex pendingJobCount_m;

    size_t nextJobIndex = 0;
};

class JobListContainer {
  public:
    JobListContainer(std::pair<std::mutex, std::shared_ptr<JobList>> &p) : jobList(p.second), lock(p.first) {
      
    };

    JobList * operator->()const { return jobList.get(); }
    JobList & operator*() const { return *jobList; }

    std::shared_ptr<JobList>& sharedPtr() {return jobList;}
  private:
    std::shared_ptr<JobList> jobList;
    std::unique_lock<std::mutex> lock;
};

#endif