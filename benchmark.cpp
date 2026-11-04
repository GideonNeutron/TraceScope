#include "tracescope.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
volatile std::uint64_t sink = 0;
void work(unsigned n) {
    std::uint64_t v = n + 1;
    for (unsigned j = 0; j < 16; ++j) v = v * 1664525ULL + 1013904223ULL;
    sink = v;
}
int main() {
    constexpr unsigned N = 200000, rounds = 9;
    tracescope::Recorder r(4096);
    auto run = [&](bool enabled) {
        const auto begin = tracescope::Clock::now();
        for (unsigned i = 0; i < N; ++i) {
            if (enabled) { tracescope::Scope s(r, "work", 0); work(i); }
            else work(i);
        }
        return std::chrono::duration<double,std::nano>(tracescope::Clock::now()-begin).count()/N;
    };
    run(false);run(true);
    std::vector<double> base, measured;
    std::cout << std::fixed << std::setprecision(3) << "round,baseline_ns_per_op,instrumented_ns_per_op\n";
    for (unsigned k=0;k<rounds;++k) {
        double b, m;
        if(k%2) {m=run(true);b=run(false);} else {b=run(false);m=run(true);}
        base.push_back(b);measured.push_back(m);
        std::cout << k+1 << ',' << b << ',' << m << '\n';
    }
    std::sort(base.begin(),base.end());std::sort(measured.begin(),measured.end());
    std::cerr << std::fixed << std::setprecision(3)
        << "{\"operationsPerRound\":" << N << ",\"rounds\":" << rounds
        << ",\"baselineMedianNs\":" << base[rounds/2]
        << ",\"instrumentedMedianNs\":" << measured[rounds/2]
        << ",\"incrementalMedianNs\":" << measured[rounds/2]-base[rounds/2]
        << ",\"eventSizeBytes\":" << sizeof(tracescope::Event)
        << ",\"capacity\":" << r.capacity()
        << ",\"eventStorageBytes\":" << r.event_storage_bytes() << "}\n";
}
