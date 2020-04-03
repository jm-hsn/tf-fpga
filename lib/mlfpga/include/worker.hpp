#ifndef myWORKER_H
#define myWORKER_H

#include <vector>
#include <condition_variable>
#include <mutex>
#include <future>

#include "jobList.hpp"
#include "commFPGA.hpp"

class Worker {
  public:
    Worker(std::vector<std::unique_ptr<commFPGA>> *fpgas);
    ~Worker();

    void startAsync();
    void startSync();
    
    int assignJobList(std::shared_ptr<JobList> &jobList);

  private:
    std::mutex currentJobList_m;
    std::shared_ptr<JobList> currentJobList = NULL;
    std::vector<std::unique_ptr<commFPGA>> *fpgaVector;

    commFPGA* findAvailableFPGA();
    void sendJob(std::shared_ptr<Job> &job);
    

    std::future<int> result;
    int threadMain();

    std::condition_variable hasJobList;
    void waitForJobList();
};

#endif