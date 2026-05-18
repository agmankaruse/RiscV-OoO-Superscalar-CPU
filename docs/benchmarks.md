# Benchmarks

Run all benchmarks with:

```bash
python tools/run_cpu_benchmarks.py
python tools/plot_cpu_results.py
```

## dependency_chain

Tests a serial dependence chain. Expected behavior is low IPC because each instruction waits on the previous producer. Demonstrates RAW dependency latency and physical-register readiness.

## branch_loop

Exercises branch prediction, mispredict recovery, and in-order retirement around a small loop. Interesting stats include branch accuracy, flush count, and branch mispredict stall cycles.

## memory_stream

Exercises repeated loads and stores. Interesting stats include D-cache hits/misses, LSQ occupancy, and cache miss stall cycles.

## independent_ilp

Contains independent arithmetic operations that can issue out of order. A good result has higher IPC and visible issue parallelism.

## load_use_latency

Highlights load-to-use delay and memory dependence behavior. Useful for inspecting LSQ and D-cache effects.

## multiply_latency

Demonstrates multi-cycle RV32M functional-unit latency and precise commit behavior when younger work may complete earlier.
