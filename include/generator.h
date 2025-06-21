
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
using namespace std;

extern size_t n = 100;
size_t seed = 42;
constexpr int base = 1e9 + 7;

parlay::sequence<int> generate_uniform(size_t range) {
  parlay::sequence<int> result(n);
  int num_workers = parlay::num_workers();

  // preventing work-stealing so the input generated is deterministic
  parlay::parallel_for(0, num_workers, [&](size_t k) {
    static thread_local std::mt19937 gen(seed + k);
    std::uniform_int_distribution<> dist(0, range);

    for (int i = n / num_workers * k; i < n / num_workers * (k+1); i++){ 
      // will this account for non-perfect partitions?
      result[i] = dist(gen);
    }
  });
  return result;
}

parlay::sequence<int> generate_exponential(double ld) {
  parlay::sequence<int> result(n);
  int num_workers = parlay::num_workers();

  parlay::parallel_for(0, num_workers, [&](size_t k) {
    static thread_local std::mt19937 gen(seed + k);
    std::exponential_distribution<> dist(ld);

    for (int i = n / num_workers * i; i < n / num_workers * (i+1); i++){ 
      // will this account for non-perfect partitions?
      result[i] = int(dist(gen));
    }
  });
  return result;
}

template<class T>
parlay::sequence<T> zipfian_generator(double s) {
  printf("zipfian distribution with s: %f\n", s);
  size_t cutoff = n;
  auto harmonic = parlay::delayed_seq<double>(cutoff, [&](size_t i) { return 1.0 / pow(i + 1, s); });
  double sum = parlay::reduce(make_slice(harmonic));
  double v = n / sum;
  parlay::sequence<size_t> nums(cutoff + 1, 0);
  parlay::parallel_for(0, cutoff, [&](size_t i) { nums[i] = max(1.0, v / pow(i + 1, s)); });
  size_t tot = scan_inplace(make_slice(nums));
  assert(tot >= n);
  parlay::sequence<T> seq(n);
  parlay::parallel_for(0, cutoff, [&](size_t i) {
    parlay::parallel_for(nums[i], min(n, nums[i + 1]), [&](size_t j) {
      // seq[j] = _hash(static_cast<T>(i));
      seq[j] = i;
    });
  });
  return random_shuffle(seq);
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