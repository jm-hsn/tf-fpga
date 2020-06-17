#include <iostream>
#include <stdio.h>
#include <assert.h> 
#include <mutex> 
#include <map>
#include <vector>
#include <random>
#include <chrono>


#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <pthread.h>
#include <string.h>

using namespace std;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds ms;
typedef std::chrono::microseconds us;

int resolvehelper(const char* hostname, int family, const char* service, sockaddr_storage* pAddr)
{
    int result;
    addrinfo* result_list = NULL;
    addrinfo hints = {};
    hints.ai_family = family;
    hints.ai_socktype = SOCK_DGRAM; // without this flag, getaddrinfo will return 3x the number of addresses (one for each socket type).
    result = getaddrinfo(hostname, service, &hints, &result_list);
    if (result == 0)
    {
        //ASSERT(result_list->ai_addrlen <= sizeof(sockaddr_in));
        memcpy(pAddr, result_list->ai_addr, result_list->ai_addrlen);
        freeaddrinfo(result_list);
    }

    return result;
}

sockaddr_storage addrDest = {};
int sock;

Clock::time_point start = Clock::now();
uint64_t packets = 0;
uint64_t bytes = 0;

int main(void) {
  int err;

  struct addrinfo hints, *res;
  
  //UDP host
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;  // use IPv4
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

  getaddrinfo(NULL, "1234", &hints, &res);
  sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  err = bind(sock, res->ai_addr, res->ai_addrlen);
  if(err != 0) {
    printf("sock: %2d, err: %2d\n", sock, err);
    exit(1);
  }
  resolvehelper("localhost", AF_INET, "1234", &addrDest);
  
  while(1) {
      uint8_t buf[1500];
      int len;
      

      uint slen = sizeof(addrDest);
      len = recvfrom(sock, buf, 1500, 0, (sockaddr*)&addrDest, &slen);
      if(len<=0)
        continue;
      sendto(sock, buf, len, 0, (sockaddr*)&addrDest, sizeof(addrDest));

      bytes += len;
      packets++;

      if(len != 1024 || packets >= 10000 || chrono::duration_cast<ms>(Clock::now() - start).count() >= 1000) {
        int64_t delay = chrono::duration_cast<us>(Clock::now() - start).count();
        start = Clock::now();
        printf("sent %7lu packets with %10lu bytes = %9lu values in %8.3f ms (%8.3f MBit/s)\n", packets, bytes, bytes/4, delay/1000.0, delay/1000.0/1000.0 * bytes / 1024.0 / 1024.0 * 8);
        packets = 0;
        bytes = 0;
      }
  }
}