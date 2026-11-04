# TraceScope

An **AI-assisted C++17 performance-instrumentation project**. It demonstrates instrumented CPU scopes, bounded storage, and reproducible measurement.

## What runs

- RAII scope timers using `std::chrono::steady_clock`.
- A preallocated, bounded circular event array with overwrite accounting.
- Mutex-protected writes from multiple producer threads.
- Named, nested duration events with thread lanes and JSON export.
- A four-thread synthetic-frame demo (4,000 frames / 8,000 recorded spans).
- Reproducible baseline-versus-instrumented measurements with raw samples.

No renderer, GPU capture, sampling, hardware counters, or console integration is implemented. This records explicitly instrumented CPU scopes; it does not automatically profile arbitrary programs.

## Build and run

Requirements: Linux, GCC with C++17 support, pthreads, and Python 3. No downloaded packages or services are required.

```bash
bash build.sh
```

This builds/runs the demo, tests and benchmark and validates the exported JSON. See `results/demo_trace.json`, `results/benchmark.csv`, `results/benchmark_summary.json`, and `results/BENCHMARK.md`. Every benchmark invocation replaces these result files with the latest measurements. The root-level `BENCHMARK.md`, `benchmark.csv`, and `benchmark_summary.json` preserve the measured run reported below; generated output is written under `results/`.

To add instrumentation:

```cpp
#include "tracescope.hpp"
tracescope::Recorder recorder(4096);
{
    tracescope::Scope timing(recorder, "update", 0);
    // Work to measure.
}
recorder.export_json("trace.json");
```

Export after producers have joined. Keep the recorder alive longer than its scopes. Names are copied into 48-byte fields and truncated at 47 bytes; use ASCII names. Caller-assigned numeric thread IDs identify lanes. JSON events use complete-duration `X` spans with microsecond timestamps/durations. The event list is in recording-completion order, which differs from start-time order for nested/concurrent scopes.

## Design and tradeoffs

A circular array bounds retained history and avoids allocation in the recording path after construction. A mutex makes multi-producer correctness straightforward but serializes concurrent writes. This is intentionally **not lock-free**. RAII ensures the span is recorded on ordinary scope exit, including stack unwinding; recording still assumes a functioning mutex. Copying names avoids dangling label pointers. Export snapshots allocate and copy the retained event array; exported metadata assumes producers are stopped.

At capacity 4,096, event-array storage is 294,912 bytes (288 KiB) on the measured host. This excludes the mutex, recorder object, allocator overhead, and export snapshots. The nine-round benchmark recorded a median baseline of 6.189 ns/op, instrumented median of 71.920 ns/op, and a difference of medians of 65.731 ns/op. These are synthetic shared-host measurements, not Nintendo hardware measurements, production overhead, or a latency guarantee. See the full environment and methodology in `BENCHMARK.md`.

Nested parent/child durations overlap. Summing them double-counts time; do not interpret the sum as total wall time. The synthetic demo has no real game loop, rendering, or frame budget. The four-thread demo verifies event capture, not throughput scaling.

## Tests

`test.cpp` checks zero-capacity rejection, chronological ring overwrite, bounded retention, exact event counts from four concurrent producers, RAII timing, label truncation, and nonnegative durations. `validate_trace.py` parses JSON, validates counts, thread lanes and escaping, and verifies all 8,000 demo events. The C++ tests pass GCC AddressSanitizer (leak detection disabled) and UndefinedBehaviorSanitizer on the provided host. LeakSanitizer cannot run in this host’s traced process environment. No race detector was run.

```bash
g++ -std=c++17 -O1 -g -fsanitize=address,undefined \
  -fno-omit-frame-pointer -pthread -I. test.cpp -o build/tests_sanitized
ASAN_OPTIONS=detect_leaks=0 ./build/tests_sanitized
```

## Files

- `tracescope.hpp`: recorder and RAII scope timer.
- `demo.cpp`: four-producer synthetic example.
- `benchmark.cpp`: warmup and nine alternating paired timing rounds.
- `test.cpp`: C++ correctness tests.
- `validate_trace.py`: exported JSON checks.
- `report.py`: benchmark environment/report generation.
- `BENCHMARK.md`, `benchmark.csv`, `benchmark_summary.json`: archived measurements.
- `results/`: generated outputs when the build script is run.
