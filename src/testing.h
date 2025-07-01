#pragma once

#include "generator.h"
#include "parlay/sequence.h"
#include "../../parlaylib/examples/samplesort.h"

#include <iostream>
#include <fstream>

using namespace std;
using namespace parlay;

constexpr int NUM_ROUNDS = 5;

template<typename T>
double test(const sequence<T> &in) {
  std::cout << "test_name: function" << std::endl;
  double total_time = 0;
  for (int i = 0; i <= NUM_ROUNDS; i++) {
    auto seq = in;
    internal::timer t;
    // function to test
    sample_sort(seq);
    t.stop();

    if (i == 0) {
      printf("Warmup: %f\n", t.total_time());
    } else {
      printf("Round %d: %f\n", i, t.total_time());
      total_time += t.total_time();
    }
  }
  double avg = total_time / NUM_ROUNDS;
  printf("Average: %f\n", avg);
  return avg;
}

template<typename T>
void run_all(const sequence<T> &seq) {
    vector<double> times;
    times.push_back(test(seq));
    printf("\n");
    ofstream ofs("results.tsv", ios::app);
  for (auto t : times) {
    ofs << t << '\t';
  }
  ofs << '\n';
  ofs.close();
}

void run_rep_dist() {
  // uniform distribution
  vector<size_t> num_keys{10000000, 1000};
  for (auto v : num_keys) {
    auto seq = generate_uniform(v);
    printf("uniform distribution with num_keys: %zu\n", v);
    run_all(seq);
  }

  // exponential distribution
  vector<double> lambda{0.00002, 0.00007};
  for (auto v : lambda) {
    auto seq = generate_exponential(v);
    printf("exponential distribution with lambda: %.10f\n", v);
    run_all(seq);
  }

  // zipfian distribution
  vector<double> s{0.8, 1.2};
  for (auto v : s) {
    auto seq = generate_zipf(v);
    printf("zipfian distribution with alpha: %f\n", v);
    run_all(seq);
  }

  // bitexp distribution
  vector<double> t{5.0};
  for (auto v : t){
    auto seq = generate_bitexp(1 / v);
    printf("bit exponential distribution with t: %f\n", v);
    run_all(seq);
  }

  // all equal
  auto equal_seq = allEqual(1);
  printf("all equal distribution\n");
  run_all(equal_seq);

  // sorted
  auto sort_seq = sorted(true);
  printf("sorted distribution\n");
  run_all(sort_seq);

  // reverse sorted
  auto reverse_seq = sorted(false);
  printf("reverse sorted distribution\n");
  run_all(reverse_seq);

  // duplicate distributions
  auto rd_seq = RootDup();
  printf("rootdup distribution\n");
  run_all(rd_seq);
  
  auto t_seq = TwoDup();
  printf("twodup sorted distribution\n");
  run_all(t_seq);

  auto e_seq = EightDup();
  printf("eightdup sorted distribution\n");
  run_all(e_seq);

  // two equal ascending arrays
  auto merge_seq = mergeDup();
  printf("eightdup sorted distribution\n");
  run_all(merge_seq);
}


void run_all_sizes() {
  vector<size_t> sizes{1'000'000, 10'000'000, 100'000'000, 1'000'000'000, 2'000'000'000, 4'000'000'000};
  for (auto input_size : sizes) {
    n = input_size;
    printf("size: %ld\n", n);
    run_rep_dist();
  }
}