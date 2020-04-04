#include <stdio.h>
#include "connectionManager.hpp"

ConnectionManager connectionManager;

size_t s=0, f=0, r=0;
std::mutex statsLk;

void work() {
    auto worker = connectionManager.createWorker(Module::dummyModule, 1);

    worker->setJobTimeout(microseconds(10000));
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

int main(void)
{
    puts("This is a shared library test...");

    
    connectionManager.addFPGA("192.168.1.32", 1234);

    //connectionManager.setSendDelay(microseconds(0));

    connectionManager.start();

    for(int i=0; i<7; i++)
        work();
    
    for(size_t i=0; i<connectionManager.getWorkerCount(); i++) {
        connectionManager.getWorker(i)->waitUntilDone();
    }

    std::unique_lock<std::mutex> lk(statsLk);
    printf("failed: %lu, successful: %lu, retries: %lu\n", f, s, r);
    return 0;
}