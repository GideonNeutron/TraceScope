#include "tracescope.hpp"
#include <atomic>
#include <iostream>
#include <thread>
int main(int argc, char** argv) {
    tracescope::Recorder recorder(8192);
    std::atomic<std::uint64_t> checksum{0};
    std::vector<std::thread> workers;
    for (unsigned t = 0; t < 4; ++t) workers.emplace_back([&, t] {
        for (unsigned n = 0; n < 1000; ++n) {
            tracescope::Scope scope(recorder, "synthetic_frame", t);
            std::uint64_t value = n + 1;
            {
                tracescope::Scope update(recorder, "update", t);
                for (unsigned j = 0; j < 400; ++j) value = value * 1664525ULL + 1013904223ULL;
            }
            checksum.fetch_xor(value, std::memory_order_relaxed);
        }
    });
    for (auto& worker : workers) worker.join();
    recorder.export_json(argc > 1 ? argv[1] : "trace.json");
    std::cout << "threads=4 frames=4000 events=" << recorder.total()
              << " retained=" << recorder.snapshot().size()
              << " storage_bytes=" << recorder.event_storage_bytes()
              << " checksum=" << checksum.load() << '\n';
}
