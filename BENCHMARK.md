# Measured results

Run: 2026-11-04T13:06:25.845369+00:00

Compiler: g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0

CPU: AMD EPYC 9V74 80-Core Processor

Flags: `-std=c++17 -O2 -pthread`

Nine timed rounds, 200,000 operations per round, after warmup. Alternating baseline/instrumented order. One producer; 16 integer recurrence steps per operation with a volatile checksum sink.

| Metric | Result |
|---|---:|
| Median baseline | 6.189 ns/op |
| Median instrumented | 71.920 ns/op |
| Difference of medians | 65.731 ns/op |
| Event size | 72 bytes |
| Ring capacity | 4096 events |
| Event array storage | 294912 bytes |

These are synthetic measurements on a shared Linux host, not console benchmarks or production overhead. The measured increment includes two steady-clock calls, two bounded name copies, mutex locking, and ring storage. Event-array size excludes the recorder object, allocator metadata, mutex, and temporary export snapshots. No allocations occur in `record()` after construction; snapshot/export allocate. With four producers the mutex serializes recording. The 4-thread demo validates correctness, not throughput scaling. Nested span totals double-count enclosing work and must not be interpreted as aggregate wall time.
