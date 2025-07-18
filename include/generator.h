#pragma once

#include "parlay/parallel.h"
#include "parlay/primitives.h"
#include "parlay/sequence.h"
#include "parlay/utilities.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <utility>
#include <random>
#include <cmath>
using namespace std;

extern size_t n = 1'024'000'000;
size_t seed = 42;
constexpr int base = 1e9 + 7;

parlay::sequence<int> generate_uniform(size_t range) {
  parlay::random_generator gen(seed);
  std::uniform_int_distribution<int> dis(0, range);

  auto result = parlay::tabulate(n, [&](size_t i) {
    auto r = gen[i];
    return dis(r);
  });

  return result;
}

parlay::sequence<int> generate_exponential(double ld) {
  parlay::random_generator gen(seed);
  std::exponential_distribution<> dis(ld);

  auto result = parlay::tabulate(n, [&](size_t i) {
    auto r = gen[i];
    return int(dis(r));
  });

  return result;
}

// currently not deterministic
parlay::sequence<int> generate_zipf(double alpha) {
  using std::pow;

  // compute Zipf probabilities
  parlay::sequence<double> probs(n);
  parlay::parallel_for(0, n, [&](size_t i) {
    probs[i] = 1.0 / pow((double)(i + 1), alpha);
  });

  // normalize
  double total = parlay::reduce(probs);
  parlay::parallel_for(0, n, [&](size_t i) {
    probs[i] /= total;
  });

  // create cdf
  parlay::sequence<double> cdf(n);
  cdf[0] = probs[0];
  parlay::parallel_for(1, n, [&](size_t i) {
    cdf[i] = cdf[i - 1] + probs[i];
  });
  cdf[n - 1] = 1.0;

  // sample
  parlay::sequence<int> result(n);
  int num_workers = parlay::num_workers();

  parlay::parallel_for(0, num_workers, [&](size_t k) {
    std::mt19937 gen(seed + k);
    std::uniform_real_distribution<> dist(0.0, 1.0);

    size_t start = n * k / num_workers;
    size_t end = n * (k + 1) / num_workers;

    for (size_t j = start; j < end; ++j) {
      double z = dist(gen);

      // Binary search for smallest index i such that cdf[i] >= z
      size_t left = 0, right = n - 1, mid;
      while (left < right) {
        mid = (left + right) / 2;
        if (cdf[mid] >= z)
          right = mid;
        else
          left = mid + 1;
      }
      result[j] = (int)(left + 1);  // Zipf values start at 1
    }
  });

  return result;
}

auto generate_bitexp(double rate){
  parlay::sequence<int> result(n);
  size_t num_bits = 4 * 8; // size_of(int) but c++ got mad at me for doing that :(
  parlay::random_generator gen(seed);
  std::uniform_real_distribution<double> dis(0, 1);

  parlay::parallel_for(0, n, [&](size_t i){
    for (int j = 0; j < num_bits; j++){
      auto r = gen[i * num_bits + j];
      if (dis(r) > rate){
        result[i] |= static_cast<int>(1) << j;
      }
    }
  });

  return result;
}

parlay::sequence<int> allEqual(int key){
  return parlay::sequence<int>(n, key);
}

// later uhh consider the things that go past 2 bil bc int won't work
auto sorted(bool asc){
  return parlay::tabulate(n, [&](size_t i){
    return asc? i : n - i;
  });
}

auto RootDup(){
  int block = static_cast<int>(sqrt(n));
  return parlay::tabulate(n, [&](size_t i){
    return i % block;
  });
}

auto TwoDup(){
  return parlay::tabulate(n, [&](size_t i){
    return (i^2 + n/2) % n;
  });
}

auto EightDup(){
  return parlay::tabulate(n, [&](size_t i){
    return (i^8 + n/2) % n;
  });
}

auto mergeDup(){
  return parlay::tabulate(n, [&](size_t i){
    return i % (n/2);
  });
}


template<class T>
void write_to_file(const parlay::sequence<pair<T, T>> &seq) {
  ofstream ofs("sequence.bin");
  ofs.write(reinterpret_cast<const char *>(seq.begin()), sizeof(pair<T, T>) * n);
  ofs.close();
}

template<class T>
parlay::sequence<pair<T, T>> read_from_file() {
  ifstream ifs("sequence.bin");
  parlay::sequence<pair<T, T>> seq(n);
  ifs.read(reinterpret_cast<char *>(seq.begin()), sizeof(pair<T, T>) * n);
  ifs.close();
  return seq;
}