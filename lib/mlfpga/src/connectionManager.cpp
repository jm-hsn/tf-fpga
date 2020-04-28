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

  addFPGA("192.168.1.33", 1234);
  addFPGA("192.168.1.34", 1234);
  addFPGA("192.168.1.35", 1234);

  start();

  printf("fpga server started\n");
}

void ConnectionManager::start() {
  running = true;
  sendResult = std::async(std::launch::async, &ConnectionManager::sendThread, this);
}

void ConnectionManager::sendThread() {
  pthread_setname_np(pthread_self(), "mlfpga send");
  std::chrono::steady_clock::time_point start;
  while(running) {
    start = std::chrono::steady_clock::now();
    for(std::vector<std::unique_ptr<commFPGA>>::iterator it=fpgas.begin(); it!=fpgas.end(); it++) {
      auto fpga = it->get();
      fpga->sendFromBuffer();
    }
    auto elapsed =std::chrono::steady_clock::now() - start;
    if(elapsed < sendDelay)
      std::this_thread::sleep_for(sendDelay - elapsed);
  }
}