#include "parlay/sequence.h"
#include "parlay/parallel.h"
#include "generator.h"
#include <random>

typedef std::mt19937 MyRNG;  // the Mersenne Twister with a popular choice of parameters
MyRNG rng; 

int main(){
    auto input1 = generate_uniform(1000);
    auto input2 = generate_uniform(1000);

    parlay::parallel_for(0, n, [&](size_t i){
        assert(input1[i] == input2[i]);
    });

    return 0;
}