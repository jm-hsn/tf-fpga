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
    Worker(std::vector<std::reference_wrapper<commFPGA>> &fpgas);
    ~Worker();

    void start();
    
    int assignJobList(JobList &jobList);

  private:
    std::mutex currentJobList_m;
    JobList *currentJobList = NULL;
    std::vector<std::reference_wrapper<commFPGA>> *fpgaVector;

    commFPGA* findAvailableFPGA();
    

    std::future<int> result;
    int threadMain();

    std::condition_variable hasJobList;
    void waitForJobList();
};

#endif