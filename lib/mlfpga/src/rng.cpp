#include "rng.hpp"

namespace RNG {
  std::random_device seed_generator;
  unsigned seed = seed_generator();
  std::uniform_int_distribution<uint32_t> distribution(0, UINT32_MAX);
  std::mt19937 mersenne_generator(seed);

  std::mutex lock;
}

uint32_t getRandomNumber() {
  std::lock_guard<std::mutex> lk(RNG::lock);
  return RNG::distribution(RNG::mersenne_generator);
}
