# RiscV-OoO-Superscalar-CPU

An educational C++17 simulator for a small RV32I out-of-order superscalar CPU. The project is a cycle-accurate-style teaching model: it does not try to synthesize hardware, but it does model the major structures that make modern cores interesting.

## Why This Project Is Impressive

Most beginner CPU simulators stop at a simple five-stage in-order pipeline. This one models a more modern out-of-order backend with register renaming, a physical register file, a reorder buffer, an issue queue, a load/store queue, multiple functional units, in-order retirement, and branch recovery. The result is still readable C++17, but the execution model demonstrates the same ideas found in serious computer architecture courses.

## Architecture

```text
                2-wide front end                         out-of-order backend

        +----------------------+       +-----------------------------------------+
        |  Instruction Fetch   | ----> | Decode                                  |
        +----------------------+       +-----------------------------------------+
                         |              |
                         v              v
        +----------------------+       +-----------------------------------------+
        | Rename Map / RAT     | ----> | Dispatch to ROB, Issue Queue, and LSQ   |
        | Free Physical Regs   |       +-----------------------------------------+
        +----------------------+              |               |              |
                         |                    v               v              v
                         |             +-------------+  +-----------+  +------------+
                         |             | Issue Queue |  |   ROB     |  |    LSQ     |
                         |             +-------------+  +-----------+  +------------+
                         |                    |
                         v                    v
                 +---------------+  +---------------+  +----------------+
                 | Integer ALU 0 |  | Integer ALU 1 |  | Branch Unit    |
                 +---------------+  +---------------+  +----------------+
                                      +----------------+
                                      | Load/Store Unit |
                                      +----------------+
                                                |
                                                v
                                    +------------------------+
                                    | Writeback / Commit     |
                                    | in program order       |
                                    +------------------------+
```

## 8-Stage OoO Macro-Pipeline

1. **Instruction Fetch**: fetches up to two instructions per cycle from the text program.
2. **Decode**: normalizes parsed RV32I-like instructions for the backend.
3. **Rename**: maps architectural registers to physical registers and removes false dependencies.
4. **Dispatch**: allocates ROB, issue queue, and LSQ entries.
5. **Issue / Select**: wakes ready instructions and selects them for available functional units.
6. **Execute**: runs integer, branch, and memory operations with simple per-unit latency.
7. **Writeback**: writes results into the physical register file and marks ROB entries ready.
8. **Commit / Retire**: retires up to two instructions per cycle in original program order.

## Register Renaming

The simulator keeps a Register Alias Table that maps architectural registers `x0..x31` to physical registers. When an instruction writes a destination register, rename allocates a fresh physical register from the free list and updates the RAT. The old physical register is kept alive until the overwriting instruction commits, which preserves precise architectural state and eliminates false WAR/WAW dependencies.

`x0` is special: it always maps to physical register `p0`, always reads as zero, and ignores writes.

## Reorder Buffer

The ROB holds all in-flight instructions in program order. Instructions may execute out of order, but they become architecturally visible only when they reach the head of the ROB and are marked ready. This gives the simulator precise retirement and a clean recovery point for wrong-path instructions.

## Issue Queue

The issue queue acts like a compact reservation station. Each entry records source physical registers, destination physical register, and the ROB id. Every cycle, ready entries compete for the available functional units:

- 2 integer ALUs
- 1 branch unit
- 1 load/store unit

Independent instructions can pass older stalled instructions when their operands are ready.

## In-Order Commit

Commit retires up to two ready ROB entries per cycle. Register-producing instructions update the committed architectural map and release the old physical register. Stores update memory only at commit, which keeps memory state precise.

## Branch Recovery

The initial predictor is static not-taken. Control instructions carry a rename-map snapshot. When a branch or jump resolves to a different next PC than predicted, the simulator:

- flushes younger ROB, issue queue, LSQ, and front-end entries
- restores the rename map to the branch recovery snapshot
- rebuilds the free physical register list
- redirects fetch to the correct target

## Supported Instructions

The first version implements this RV32I subset:

- `ADD`, `SUB`
- `AND`, `OR`, `XOR`
- `SLL`, `SRL`, `SRA`
- `ADDI`, `ANDI`, `ORI`, `XORI`
- `LW`, `SW`
- `BEQ`, `BNE`, `BLT`, `BGE`
- `JAL`, `JALR`
- `LUI`, `AUIPC`
- `NOP`

Programs use a simple assembly-like text format with labels, comments, and blank lines.

```asm
ADDI x1, x0, 5
ADDI x2, x0, 10
ADD  x3, x1, x2
SW   x3, 0(x0)
LW   x4, 0(x0)
BEQ  x3, x4, label
ADDI x5, x0, 99
label:
ADD  x6, x3, x4
```

## Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Run Instructions

Normal mode:

```bash
./riscv_ooo_sim ../examples/arithmetic_program.txt
```

Trace mode:

```bash
./riscv_ooo_sim --trace ../examples/arithmetic_program.txt
```

On Windows with a multi-config generator, the executable may be under `build/Debug/` or `build/Release/`.

## Test Instructions

```bash
cd build
ctest --output-on-failure
```

The tests are assert-based and cover arithmetic, dependencies, out-of-order readiness, load/store behavior, branch recovery, and `x0` correctness.

## Example Trace Output

```text
cycle 1
  fetch pc=0 ADDI x1, x0, 5
  fetch pc=1 ADDI x2, x0, 10
cycle 2
  decode ADDI x1, x0, 5
  decode ADDI x2, x0, 10
cycle 3
  rename ADDI x1, x0, 5
  rename ADDI x2, x0, 10
cycle 4
  dispatch ROB1 ADDI x1, x0, 5
  dispatch ROB2 ADDI x2, x0, 10
cycle 5
  issue ROB1 ADDI x1, x0, 5
  issue ROB2 ADDI x2, x0, 10
...
cycles=12 retired=6 ipc=0.50 branch_mispredicts=0 rob=0/4 iq=0/4 lsq=0/0
```

## Future Improvements

- RV32M support
- Better branch predictor
- Cache simulator
- ELF loader
- Superscalar width configuration
- Tomasulo-style visualization
- Web-based pipeline viewer
