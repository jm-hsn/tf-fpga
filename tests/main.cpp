#include <stdio.h>
#include "connectionManager.hpp"

ConnectionManager connectionManager;

Module mod = Module::dummyBigModule;

size_t s=0, f=0, r=0;
std::mutex statsLk;

void work() {
    auto worker = connectionManager.createWorker(mod, 1000);

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

int main(void)
{
    puts("This is a shared library test...");

    
    connectionManager.addFPGA("192.168.1.33", 1234);
    connectionManager.addFPGA("192.168.1.34", 1234);
    connectionManager.addFPGA("192.168.1.35", 1234);

    connectionManager.setSendDelay(microseconds(50));

    connectionManager.start();

    int workNum = 10000;
    int n=1;
    
    while(workNum > 0 || connectionManager.getWorkerCount() > 0) {
        std::this_thread::sleep_for(milliseconds(300));
        connectionManager.removeFinishedWorkers();
        while(workNum > 0 && connectionManager.getWorkerCount() < 8) {
            workNum--;
            work();
        }
        std::unique_lock<std::mutex> lk(statsLk);
        printf("work: %2d   worker: %2lu failed: %12lu, successful: %12lu, retries: %12lu  %4lu MBit/s\n", workNum, connectionManager.getWorkerCount(), f, s, r, s*(moduleSendPayloadLength[mod]+4)*4*10*8/1024/1024/3/(n++));
    }
    return 0;
}