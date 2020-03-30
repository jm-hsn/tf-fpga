#include "../include/job.hpp"

WordBuffer::WordBuffer(size_t length) {
  wordCount = length;
  bytes = (uint8_t*)malloc(wordCount * 4);
  assert(bytes != NULL);
}

WordBuffer::~WordBuffer() {
  free(bytes);
}

JobData::JobData(uint payloadLength) : WordBuffer(payloadLength + 4) {
  setPreamble(PREAMBLE);
}

Job::Job(Module mod) : JobData(moduleSendPayloadLength[mod]), recvBuf(moduleRecvPayloadLength[mod] + 1) {
  setModuleId(moduleIds[mod]);
  setJobId(getRandomNumber());
}

//sets CRC of sendBuf
void Job::calcCRC() {
  uint32_t sum = 0;
  for(uint_least32_t i=1; i<getWordCount()-1; i++) {
    sum += getWord(i);
  }
  setCRC(-sum);
}

//checks CRC of recvBuf
bool Job::checkCRC() {
  uint32_t sum = getPreamble() + getJobId() + getModuleId();
  for(uint_least32_t i=1; i<recvBuf.getWordCount()-1; i++) {
    sum += recvBuf.getWord(i);
  }
  return recvBuf.getWord(recvBuf.getWordCount()-1) == -sum;
}

void Job::setDoneCallback(DoneCallback cb) {
  doneCb = cb;
}

void Job::isComplete() {
  received = Clock::now();
  if(doneCb)
    doneCb();
}