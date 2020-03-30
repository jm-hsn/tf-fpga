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
    void finishJob();

    void setDoneCallback(DoneCallback cb);

    size_t getPendingJobCount() const {return pendingJobCount;}

    Job& getJob(size_t i);

    Job* getNextJob();
  private:
    std::vector<std::reference_wrapper<Job>> jobs;
    DoneCallback doneCb;

    size_t jobCount;
    size_t pendingJobCount;
    std::condition_variable jobListDone;
    std::mutex pendingJobCount_m;

    size_t nextJobIndex = 0;
};

#endif