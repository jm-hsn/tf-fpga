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
    Worker(std::vector<std::unique_ptr<commFPGA>> *fpgas, Module mod, size_t numberOfJobs);
    ~Worker();

    void startAsync();
    void startSync();
    
    JobListContainer getJobList();

    void setJobTimeout(microseconds us) {jobTimeout = us;}
    void setRetryCount(size_t n) {retryCount = n;}

    void setDoneCallback(DoneCallback cb);
    void waitUntilDone();

  private:
    std::pair<std::mutex, std::shared_ptr<JobList>> jobList;
    std::vector<std::unique_ptr<commFPGA>> *fpgaVector;

    commFPGA* findAvailableFPGA();
    
    std::future<int> result;
    int threadMain();

    microseconds jobTimeout = microseconds(1000);
    size_t retryCount = 10;

    DoneCallback doneCb = NULL;
    bool running = true;
};

#endif