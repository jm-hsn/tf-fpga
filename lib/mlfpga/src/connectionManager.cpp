#include "connectionManager.hpp"

ConnectionManager::ConnectionManager() {
  
}
ConnectionManager::~ConnectionManager() {
  running = false;
}

void ConnectionManager::addFPGA(const char* ip, const uint port, bool bindSelf) {
  fpgas.emplace_back(new commFPGA(ip, port));
  fpgas.back()->start();
}

int ConnectionManager::sendJobListAsync(std::shared_ptr<JobList> &jobList) {
  workers.emplace_back(new Worker(&fpgas));
  workers.back()->assignJobList(jobList);
  workers.back()->startAsync();
  return 0;
}
int ConnectionManager::sendJobListSync(std::shared_ptr<JobList> &jobList) {
  workers.emplace_back(new Worker(&fpgas));
  workers.back()->assignJobList(jobList);
  workers.back()->startSync();
  return 0;
}

void ConnectionManager::start() {
  sendResult = std::async(std::launch::async, &ConnectionManager::sendThread, this);
}

void ConnectionManager::sendThread() {
  while(running) {
    Clock::time_point start = Clock::now();
    for(std::vector<std::unique_ptr<commFPGA>>::iterator it=fpgas.begin(); it!=fpgas.end(); it++) {
      it->get()->sendFromBuffer();
    }
    //printf("%8d %8d\n", fpgas[0].sendBufferWriteIndex, fpgas[0].sendBufferReadIndex);
    uint us = std::chrono::duration_cast<microseconds>(Clock::now() - start).count();
    if(us < 50)
      usleep(50 - us);
  }
}