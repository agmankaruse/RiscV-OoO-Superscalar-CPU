# RISC-V Out-of-Order Superscalar CPU Simulator

A C++17 cycle-level RISC-V CPU simulator focused on the core mechanisms behind modern out-of-order execution: register renaming, physical registers, reorder-buffer retirement, issue queues, load/store ordering, branch prediction and recovery, cache stalls, timeline tracing, and differential verification.

This is a portfolio-oriented computer architecture project. It is intentionally small enough to read, but complete enough to demonstrate the correctness and performance questions that real microarchitecture simulators must answer.

## Architecture

```text
Fetch -> Decode -> Rename -> Dispatch -> Issue -> Execute -> Writeback -> Commit
                    |            |          |          |             |
                    v            v          v          v             v
              Rename Table      ROB        IQ         FUs        Arch State
                    |            |          |
                    v            v          v
            Physical Regs       LSQ      Branch Predictor
```

The simulator models an RV32I-style assembly subset with RV32M multiply/divide operations where implemented. The out-of-order backend keeps precise architectural state by committing through the ROB in program order.

## Key Features

- 8-stage macro-pipeline: fetch, decode, rename, dispatch, issue, execute, writeback, commit
- Superscalar fetch/decode/rename/dispatch/issue widths configured through JSON
- Register renaming with a physical register file and free list
- Reorder buffer with in-order retirement and branch recovery snapshots
- Issue queue and multiple functional units
- Load/store queue with store-to-load forwarding and in-order store commit
- Direct-mapped I-cache and D-cache latency modeling
- Branch predictor and mispredict flush handling
- Cycle-level stats, CPI/stall breakdowns, and CSV tracing
- In-order reference CPU for differential testing
- Invariant checker for ROB, IQ, LSQ, rename, free-list, and x0 rules
- Random program generation and randomized differential testing
- Local static HTML pipeline timeline viewer
- Benchmark and report scripts

## Why It Matters

Out-of-order execution is one of the central ideas behind high-performance CPUs. This project shows how speculative execution can improve instruction-level parallelism while preserving precise architectural results through register renaming and ROB commit. The verification layer demonstrates how to test a timing model against a simpler architectural reference model.

## Build

```bash
cmake -S . -B build -DENABLE_TESTS=ON -DENABLE_WARNINGS=ON
cmake --build build
```

Useful CMake options:

- `ENABLE_WARNINGS=ON`
- `ENABLE_ASAN=ON` on non-MSVC compilers
- `ENABLE_TRACE=ON`
- `ENABLE_TESTS=ON`

## Run

```bash
./build/riscv_ooo_sim examples/arithmetic_program.txt
./build/riscv_ooo_sim --trace examples/dependency_program.txt
./build/riscv_ooo_sim --timeline examples/dependency_program.txt
./build/riscv_ooo_sim --version
```

On Windows with a multi-config generator, the executable may be under `build/Debug/riscv_ooo_sim.exe`.

## Verification

```bash
./build/riscv_ooo_sim --diff examples/arithmetic_program.txt
./build/riscv_ooo_sim --check-invariants examples/dependency_program.txt
./build/riscv_ooo_sim --dump-on-fail --diff examples/branch_program.txt
python tools/run_random_diff_tests.py --count 20 --instructions 100
```

`--diff` runs the out-of-order simulator and the in-order reference CPU on the same program, then compares architectural registers, touched memory words, final PC, and retired instruction counts.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

The suite covers arithmetic, dependency scheduling, load/store behavior, branches, caches, RV32M operations, x0 behavior, reference execution, differential testing, invariant checking, branch flush correctness, precise commit, memory ordering, and random-program smoke coverage.

## Benchmarks

```bash
python tools/run_cpu_benchmarks.py
python tools/plot_cpu_results.py
```

Benchmark CSV output is written to `outputs/cpu_benchmark_results.csv`. Timeline CSV files are written under `outputs/timelines/`.

## Visualization

```bash
./build/riscv_ooo_sim --timeline --timeline-csv outputs/cpu_pipeline_timeline.csv examples/dependency_program.txt
```

Open `viewer/cpu_timeline_viewer.html` in a browser and select the generated CSV. The viewer works locally with no external web dependencies.

## Project Structure

- `include/`, `src/`: simulator core
- `tests/`: assert-based CTest programs
- `examples/`: small hand-written programs
- `benchmarks/`: architecture-focused benchmark programs
- `tools/`: random testing, benchmark, and plotting scripts
- `viewer/`: static pipeline timeline viewer
- `docs/`: architecture, verification, benchmarks, diagrams, limitations
- `results/`: generated example outputs

## Limitations

This is a focused educational simulator, not a full RISC-V system emulator. It models a useful RV32I/RV32M subset, sparse word-oriented memory behavior, simplified cache timing, and an intentionally compact branch predictor and LSQ. See `docs/limitations.md` for the current boundary.

## Future Work

- Add more RV32I load/store widths and system instructions
- Expand predictor policies and recovery statistics
- Add richer memory dependence prediction
- Add golden trace comparison at every commit
- Export richer JSON traces for external visualization
- Add larger benchmark kernels and perf regression thresholds


## What I Learned

This project reinforced how register renaming removes false dependencies, how ROB commit preserves precise state, how out-of-order issue interacts with readiness and functional-unit latency, how branch recovery must restore speculative rename state, how cache and resource stalls shape CPI, and how differential testing catches correctness bugs that pure performance traces miss.
