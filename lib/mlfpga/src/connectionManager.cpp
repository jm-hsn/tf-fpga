#include "connectionManager.hpp"

ConnectionManager::ConnectionManager() {
  
}
ConnectionManager::~ConnectionManager() {
  running = false;
  sendResult.get();
}

void ConnectionManager::addFPGA(const char* ip, const uint port) {
  commFPGA fpga(ip, port);
  fpga.start();

  fpgas.push_back(fpga);
}

int ConnectionManager::sendJobListAsync(JobList &jobList) {
  Worker worker(fpgas);
  worker.assignJobList(jobList);
  worker.start();
  workers.push_back(worker);
  return 0;
}

void ConnectionManager::start() {
  sendResult = std::async(std::launch::async, &ConnectionManager::sendThread, this);
}

void ConnectionManager::sendThread() {
  while(running) {
    Clock::time_point start = Clock::now();
    for(std::vector<std::reference_wrapper<commFPGA>>::iterator it=fpgas.begin(); it!=fpgas.end(); it++) {
      it->get().sendFromBuffer();
    }
    //printf("%8d %8d\n", fpgas[0].sendBufferWriteIndex, fpgas[0].sendBufferReadIndex);
    uint us = std::chrono::duration_cast<microseconds>(Clock::now() - start).count();
    if(us < 50)
      usleep(50 - us);
  }
}