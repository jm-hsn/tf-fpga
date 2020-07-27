#include <stdio.h>
#include "connectionManager.hpp"

ConnectionManager connectionManager;

Module mod = Module::conv2D_5x5_Module;
unsigned int jobsPerWorker = 100;

size_t s=0, f=0, r=0;
std::mutex statsLk;

void work() {
    auto worker = connectionManager.createWorker(mod, jobsPerWorker);

    worker->setJobTimeout(milliseconds(1000));
    worker->setRetryCount(10);
    worker->setDoneCallback([worker](){
        auto jobs = worker->getJobList();
        std::unique_lock<std::mutex> lk(statsLk);
        for(size_t i=0; i<jobs->getJobCount(); i++) {
            auto job = jobs->getJob(i);
            if(job->getState() == JobState::finished) {
                s++;
            } else if(job->getState() == JobState::failed) {
                f++;
            } else {
                printf("job %08X: invalid state %d\n", job->getJobId(), (int)job->getState());
            }
            r += job->getSendCounter() - 1;
        }
        
    });
    {
        auto jobs = worker->getJobList();
        for(size_t i=0; i<jobs->getJobCount(); i++) {
            auto job = jobs->getJob(i);
            static int num=0;
            job->setPayload(0, num);
            job->setPayload(1, num);
            job->setPayload(2, num);
            job->setPayload(3, num++);
            job->setReady();
        }
    }

    worker->startAsync();
}

int main(int argc, char *argv[])
{
    puts("This is a shared library test...");

    unsigned int numFPGA = 3;
    unsigned int workNum = 100;
    unsigned int workerCount = 1;

    if(argc > 1)
        numFPGA = atoi(argv[1]);
    
    if(numFPGA >= 1)
        connectionManager.addFPGA("192.168.1.33", 1234);
    if(numFPGA >= 2)
        connectionManager.addFPGA("192.168.1.34", 1234);
    if(numFPGA >= 3)
        connectionManager.addFPGA("192.168.1.35", 1234);

    connectionManager.setSendDelay(microseconds(50));
    connectionManager.start();

    if(argc > 2)
        workNum = atoi(argv[2]);

    if(argc > 3)
        workerCount = atoi(argv[3]);

    if(argc > 4)
        jobsPerWorker = atoi(argv[4]);
    
    printf("arguments: <numFPGA = %u> <workNum = %u> <workerCount = %u> <jobsPerWorker = %u>\n", numFPGA, workNum, workerCount, jobsPerWorker);

    while(workNum > 0 || connectionManager.getWorkerCount() > 0) {
        std::this_thread::sleep_for(microseconds(1000));
        connectionManager.removeFinishedWorkers();
        while(workNum > 0 && connectionManager.getWorkerCount() < workerCount) {
            workNum--;
            work();
            std::unique_lock<std::mutex> lk(statsLk);
            printf("work: %2d   worker: %2lu failed: %12lu, successful: %12lu, retries: %12lu\n", workNum, connectionManager.getWorkerCount(), f, s, r);
        }
    }
        std::unique_lock<std::mutex> lk(statsLk);
        printf("work: %2d   worker: %2lu failed: %12lu, successful: %12lu, retries: %12lu\n", workNum, connectionManager.getWorkerCount(), f, s, r);
    return 0;
}