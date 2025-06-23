#include "parlay/sequence.h"
#include "parlay/parallel.h"
#include "generator.h"
#include <random>

int main(){
    auto input1 = sorted();
    auto input2 = sorted();

    parlay::parallel_for(0, n, [&](size_t i){
        assert(input1[i] == input2[i]);
    });

    for (int i = 0; i < n; i++){
        cout << input1[i] << " ";
    }

    return 0;
}