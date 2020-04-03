#include <stdio.h>
#include "connectionManager.hpp"

ConnectionManager connectionManager;


int main(void)
{
    puts("This is a shared library test...");


    JobData x(4);
    printf("%08X\n", x.getPreamble());
    return 0;
    
    connectionManager.addFPGA("192.168.1.10", 1234, true);

    connectionManager.start();

    std::shared_ptr<JobList> jobs(new JobList(Module::dummyModule, 1));
    for(size_t i=0; i<1; i++)
        jobs->getJob(i)->setReady();

    connectionManager.sendJobListSync(jobs);

    return 0;
}