# RiscV-OoO-Superscalar-CPU

An educational C++17 simulator for a RISC-V out-of-order superscalar CPU. This is a cycle-accurate-style architecture model, not a hardware implementation and not a plain instruction interpreter. The simulator is designed to make modern CPU concepts visible: superscalar fetch/decode, register renaming, out-of-order issue, multiple functional units, branch prediction, cache stalls, ROB retirement, load/store ordering, and timeline traces for visualization.

## Why This Is More Than a Toy Emulator

Many small CPU projects execute one instruction at a time or model a five-stage in-order pipeline. This project models the structures that make modern cores interesting:

- configurable frontend/backend widths
- physical register file and rename table
- free physical register list
- reorder buffer with in-order commit
- issue queue with wakeup/select behavior
- load/store queue with store-to-load forwarding constraints
- direct-mapped L1 instruction and data caches
- static and dynamic branch predictors
- multi-cycle RV32M multiply/divide operations
- CSV timeline output for pipeline visualization
- occupancy statistics for ROB, IQ, LSQ, and physical registers

## Architecture Diagram

```text
Fetch -> Decode -> Rename -> Dispatch -> Issue Queue -> Functional Units -> Writeback -> ROB Commit
                             |                              |
                             v                              v
                        Rename Table                 Physical Reg File
                             |
                             v
                            ROB

L1 I-cache feeds fetch. L1 D-cache sits behind the load/store unit.
The branch predictor redirects fetch, and branch recovery flushes wrong-path work.
```

## Out-of-Order Execution Flow

The simulator keeps the original 8-stage macro-pipeline:

1. **Instruction Fetch**: fetches from the program stream through the L1 I-cache.
2. **Decode**: moves parsed RV32I/RV32M instructions into frontend queues.
3. **Rename**: maps architectural registers to physical registers.
4. **Dispatch**: allocates ROB, issue queue, and LSQ entries.
5. **Issue / Select**: chooses ready instructions for available functional units.
6. **Execute**: runs ALU, branch, memory, MUL, and DIV operations with latency.
7. **Writeback**: writes results to the physical register file.
8. **Commit / Retire**: retires ready instructions in program order.

Independent instructions can issue around older cache misses or long-latency multiply/divide work when their operands are ready and a suitable functional unit is free.

## Register Renaming Example

```asm
ADDI x1, x0, 1   # x1 -> p32
ADD  x2, x1, x1  # reads p32, x2 -> p33
ADDI x1, x0, 9   # x1 -> p34, older p32 still feeds x2
```

The rename table removes false dependencies by giving each write a fresh physical register. `x0` is permanently mapped to physical register `p0`, always reads as zero, and ignores writes.

## ROB Example

The reorder buffer preserves precise architectural state. Instructions may finish out of order, but commit happens from the ROB head only:

```text
ROB0: LW   x1, 0(x0)   waiting on D-cache miss
ROB1: ADDI x2, x0, 7   ready, cannot commit before ROB0
ROB2: ADDI x3, x0, 8   ready, cannot commit before ROB0
```

Once ROB0 completes, the commit stage can retire multiple ready entries in order.

## Issue Queue And Wakeup/Select

The issue queue stores renamed instructions with source physical registers. Each cycle, ready entries are selected for:

- 2 integer ALUs
- 1 branch unit
- 1 load/store unit

The issue width is configurable, so the model can demonstrate narrow debug cores or wider superscalar issue.

## Load/Store Queue Behavior

The LSQ tracks memory operations until retirement. Stores write memory only at commit. Loads can issue when all older store addresses are known; if an older store has the same address and its data is ready, the load forwards from that store instead of reading memory. D-cache misses hold the load/store unit for the configured miss latency while independent ALU work can continue.

## Branch Prediction And Recovery

Supported predictors:

- `static_not_taken`
- `static_taken`
- `one_bit`
- `two_bit`
- `btb`
- `return_stack`

The simulator tracks total branch predictions, correct predictions, mispredictions, accuracy, and estimated misprediction penalty cycles. A misprediction flushes younger ROB, issue queue, LSQ, and frontend work, restores the rename table from a branch snapshot, rebuilds the free-list, and redirects fetch.

## Cache Model

The L1 instruction cache and L1 data cache are direct-mapped. They support configurable size, line size, hit latency, and miss latency.

Tracked cache stats:

- I-cache hits and misses
- I-cache hit rate
- D-cache hits and misses
- D-cache hit rate
- fetch miss stalls
- load/store miss stalls

## Functional Unit Latency Model

Default latencies:

- integer ALU: 1 cycle
- branch: 1 cycle
- load/store: cache dependent
- `MUL` / `MULH`: 3 cycles
- `DIV` / `REM`: 12 cycles

Latency values are configurable from the config files.

## Configuration

Example:

```bash
./riscv_ooo_sim --config configs/default_2wide.json examples/arithmetic_program.txt
```

Provided configs:

- `configs/default_2wide.json`
- `configs/wide_4issue.json`
- `configs/tiny_debug.json`

The parser supports a small flat JSON-like format and key/value style entries without external dependencies.

## Timeline CSV Output

```bash
./riscv_ooo_sim --timeline examples/dependency_program.txt
```

This writes:

```text
outputs/pipeline_timeline.csv
```

CSV columns:

```text
cycle,instruction_id,pc,instruction,stage,event,rob_index,physical_dest
```

Events include `FETCH`, `DECODE`, `RENAME`, `DISPATCH`, `ISSUE`, `EXECUTE_START`, `EXECUTE_DONE`, `WRITEBACK`, `COMMIT`, and `FLUSHED`.

## Occupancy Stats CSV

```bash
./riscv_ooo_sim --stats-csv outputs/stats.csv examples/arithmetic_program.txt
```

CSV columns:

```text
cycle,rob_occupancy,iq_occupancy,lsq_occupancy,free_phys_regs,committed,inflight
```

## Benchmark Suite

Benchmarks live in `benchmarks/`:

- `dependency_chain.riscv`: demonstrates RAW dependency bottlenecks
- `independent_ilp.riscv`: demonstrates independent instruction-level parallelism
- `branch_loop.riscv`: demonstrates predictor warmup on loop branches
- `load_use_latency.riscv`: demonstrates load-use latency and independent work
- `memory_stream.riscv`: demonstrates D-cache behavior
- `multiply_latency.riscv`: demonstrates RV32M multi-cycle latency

Example:

```bash
./riscv_ooo_sim --config configs/wide_4issue.json --stats-csv outputs/ilp_stats.csv benchmarks/independent_ilp.riscv
```

## Example Performance Stats

A normal run prints a compact summary:

```text
cycles=28 retired=8 ipc=0.29 branch_accuracy=75.00% branch_mispredicts=1 icache_hits=7 icache_misses=2 dcache_hits=1 dcache_misses=1 fetch_miss_stalls=8 load_miss_stalls=7 rob=0/6 iq=0/4 lsq=0/1
```

The exact values depend on the selected config, cache sizes, and benchmark.

## Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Run Instructions

```bash
./riscv_ooo_sim examples/arithmetic_program.txt
./riscv_ooo_sim --trace examples/arithmetic_program.txt
./riscv_ooo_sim --config configs/default_2wide.json examples/arithmetic_program.txt
./riscv_ooo_sim --timeline examples/dependency_program.txt
./riscv_ooo_sim --stats-csv outputs/stats.csv benchmarks/branch_loop.riscv
```

On Windows with a multi-config generator, the executable may be under `build/Debug/` or `build/Release/`.

## Test Instructions

```bash
cd build
ctest --output-on-failure
```

The test suite covers arithmetic, dependencies, out-of-order execution, loads/stores, branch recovery, `x0`, branch predictor behavior, cache counters, RV32M instructions, timeline generation, and config parsing.

## Supported Instructions

RV32I subset:

- `ADD`, `SUB`
- `AND`, `OR`, `XOR`
- `SLL`, `SRL`, `SRA`
- `ADDI`, `ANDI`, `ORI`, `XORI`
- `LW`, `SW`
- `BEQ`, `BNE`, `BLT`, `BGE`
- `JAL`, `JALR`
- `LUI`, `AUIPC`
- `NOP`

RV32M subset:

- `MUL`
- `MULH`
- `DIV`
- `REM`

## Future Work

- set-associative cache model
- non-blocking cache with MSHRs
- richer memory consistency experiments
- ELF loader
- configurable functional unit counts
- tournament branch predictor
- Tomasulo-style visualization
- web-based pipeline viewer backed by the timeline CSV
