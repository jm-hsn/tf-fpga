#include "../include/job.hpp"

// jobData members


jobData::jobData(uint payloadLength) {
  wordCount = payloadLength + 4;
  bytes = (uint8_t*)malloc(wordCount * 4);
  assert(bytes != NULL);

  //set all the pointers
  jobId = &words[1];
  moduleId = &words[2];
  payload = &words[3];
  crc = &words[wordCount-1];

  //insert constant values
  *preamble = PREAMBLE;
}
jobData::~jobData() {
  free(bytes);
}

void jobData::calcCRC() {
  uint32_t sum = 0;
  for(uint_least32_t i=1; i<wordCount-1; i++) {
    sum += words[i];
  }
  *crc = -sum;
}

bool jobData::checkCRC() {
  uint32_t sum = 0;
  for(uint_least32_t i=1; i<wordCount-1; i++) {
    sum += words[i];
  }
  return *crc == -sum;
}