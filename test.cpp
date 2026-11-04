#include "tracescope.hpp"
#include <cassert>
#include <iostream>
#include <thread>
int main() {
    bool rejected = false;
    try { tracescope::Recorder invalid(0); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    tracescope::Recorder ring(3);
    for (int i = 0; i < 5; ++i) ring.record("event", i, i, 0);
    auto e = ring.snapshot();
    assert(e.size() == 3 && e[0].start_ns == 2 && e[2].start_ns == 4 && ring.total() == 5);
    tracescope::Recorder concurrent(4096);
    std::vector<std::thread> threads;
    for (unsigned t = 0; t < 4; ++t) threads.emplace_back([&, t]{
        for (int i = 0; i < 2000; ++i) { tracescope::Scope s(concurrent, "work", t); }
    });
    for (auto& t : threads) t.join();
    assert(concurrent.total() == 8000 && concurrent.snapshot().size() == 4096);
    for (const auto& x : concurrent.snapshot()) assert(x.thread < 4 && x.duration_ns >= 0);
    concurrent.record("quote\" slash\\ newline\n", 1, -4, 7);
    concurrent.record("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZlong", 2, 3, 8);
    auto final = concurrent.snapshot();
    assert(final.back().name[47] == '\0' && final[final.size()-2].duration_ns == 0);
    concurrent.export_json("results/test_trace.json");
    std::cout << "PASS: zero capacity, overwrite order, bounded storage, multi-thread counts, RAII scopes, truncation, negative duration\n";
}
