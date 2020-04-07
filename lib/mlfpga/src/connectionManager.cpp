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

Worker* ConnectionManager::createWorker(Module mod, size_t numberOfJobs) {
  Worker *w = new Worker(&fpgas, mod, numberOfJobs);
  workers.emplace_back(w);
  return w;
}

void ConnectionManager::removeFinishedWorkers() {
  workers.erase(
    std::remove_if(
      workers.begin(),
      workers.end(),
      [&] (std::unique_ptr<Worker> const& p) {
        return !p.get()->isRunning();
      }),
    workers.end()
  );
}

void ConnectionManager::startFromTensorflow() {
  if(isRunning())
    return;

  addFPGA("localhost", 1234);
  start();

  printf("fpga server started\n");
  /*
  cm.addFPGA("192.168.88.32", 1234);
  cm.addFPGA("192.168.88.33", 1234);
  cm.addFPGA("192.168.88.34", 1234);
  cm.addFPGA("192.168.88.35", 1234);
  */
}

void ConnectionManager::start() {
  running = true;
  sendResult = std::async(std::launch::async, &ConnectionManager::sendThread, this);
}

void ConnectionManager::sendThread() {
  pthread_setname_np(pthread_self(), "mlfpga send");
  while(running) {
    Clock::time_point start = Clock::now();
    for(std::vector<std::unique_ptr<commFPGA>>::iterator it=fpgas.begin(); it!=fpgas.end(); it++) {
      auto fpga = it->get();
      fpga->sendFromBuffer();
    }
    auto elapsed = Clock::now() - start;
    if(elapsed < sendDelay)
      std::this_thread::sleep_for(sendDelay - elapsed);
  }
}