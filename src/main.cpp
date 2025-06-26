#include "parlay/sequence.h"
#include "parlay/parallel.h"
#include "parlay/random.h"
#include "generator.h"
#include "heaptree.h"

#include <random>
#include <cmath>
#include <iostream>
#include <fstream>

int main(){
    int num_workers = parlay::num_workers();
    vector<long> results;

    for (int i = 0; i < n; i++){
        seed = i * num_workers;
        auto input = generate_uniform(n);

        // generate sample
        parlay::random_generator gen;
        std::uniform_int_distribution<long> dis(0, n-1);
        int num_buckets = sqrt(n);

        int over_ratio = 8;
        auto oversample = parlay::tabulate(num_buckets * over_ratio, [&] (long i) {
            auto r = gen[i];
            return input[dis(r)];
        });
        std::sort(oversample.begin(), oversample.end());

        // sub sample to pick final pivots (num_buckets - 1 of them)
        auto pivots = parlay::tabulate(num_buckets-1, [&] (long i) {
            return oversample[(i+1)*over_ratio];
        });

        // get bucket counts
        parlay::sequence<int> buckets(num_buckets * num_workers, 0);
        heap_tree ss(pivots);
        int block = n / num_workers;

        parlay::parallel_for(0, num_workers, [&](size_t i){
            for (int j = i * block; j < (i+1) * block; j++){
                buckets[i * num_buckets + ss.find(input[j], std::less())]++;
            }
        });

        auto counts = parlay::tabulate(num_buckets, [&](size_t i){
            int sum = 0;
            for (int j = 0; j < num_workers; j++){
                sum += buckets[j * num_buckets + i];
            }
            return sum;
        });
        parlay::scan_inplace(counts);

        auto unoverlapped = parlay::tabulate(num_buckets, [&](size_t i){
            int start, end;
            if (i != num_buckets - 1) {
                start = counts[i];
                end = counts[i + 1] - 1;
            } else if (i == num_buckets - 1) {
                start = counts[num_buckets-1];
                end = n - 1;
            }

            int block = static_cast<int>(sqrt(n));

            return (std::max<int>({0, std::min<int>(i * block - start, counts[i+1] - counts[i])}) + std::max<int>({0, std::min<int>(end - (i + 1) * block + 1, counts[i+1] - counts[i])}));
        });

        // for (auto v : unoverlapped){
        //     cout << v << ' ';
        // }

        results.push_back(parlay::reduce(unoverlapped));
    }

    // write to file
    ofstream ofs("results.tsv", ios::app);
    for (auto t : results) {
        cout << t << '\n';
        ofs << t << '\n';
    }

    return 0;
}