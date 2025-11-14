#pragma once

#include <random>

namespace rt::core {

// -----------------------------------------------------------------------------
// THREAD-LOCAL RANDOM ENGINE
//
// Each thread gets its own RNG so that parallel rendering (OpenMP, TBB, std::thread)
// does not cause contention or correlation.
// -----------------------------------------------------------------------------

inline std::mt19937& GetRng() {
  thread_local static std::mt19937 rng(std::random_device{}());
  return rng;
}

// -----------------------------------------------------------------------------
// Generate uniform double in [0, 1)
// -----------------------------------------------------------------------------
inline double RandomDouble() {
  thread_local static std::uniform_real_distribution<double> dist(0.0, 1.0);
  return dist(GetRng());
}

// -----------------------------------------------------------------------------
// Generate uniform double in [min, max)
// -----------------------------------------------------------------------------
inline double RandomDouble(double min, double max) {
  return min + (max - min) * RandomDouble();
}

// -----------------------------------------------------------------------------
// Generate uniform integer in [min, max]
// -----------------------------------------------------------------------------
inline int RandomInt(int min, int max) {
  std::uniform_int_distribution<int> dist(min, max);
  return dist(GetRng());
}

// -----------------------------------------------------------------------------
// Seed control (optional)
// Call this if you want deterministic rendering sequences.
// -----------------------------------------------------------------------------
inline void SeedRng(unsigned int seed) {
  GetRng().seed(seed);
}

}  // namespace rt::core
