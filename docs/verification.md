# Verification

The verification layer uses three complementary checks.

## Reference CPU

`ReferenceCPU` executes the same assembly-like programs one instruction at a time in program order. It ignores timing and maintains only architectural registers, memory, PC, and retired instruction count.

## Differential Testing

```bash
./build/riscv_ooo_sim --diff examples/arithmetic_program.txt
```

The diff mode runs both the OoO simulator and the reference CPU, then compares:

- architectural registers `x0` through `x31`
- memory words touched by either model
- final PC
- retired instruction counts

Failures print the program name, final PCs, retired counts, and each mismatched register or memory word. `--dump-on-fail` adds ROB, issue queue, rename table, physical register, LSQ, and pipeline dumps.

## Invariant Checking

```bash
./build/riscv_ooo_sim --check-invariants examples/dependency_program.txt
```

The invariant checker runs after each cycle and checks x0 behavior, physical register mapping ranges, free-list exclusion, ROB ordering, issue queue ROB references, LSQ ROB references, capacity limits, and readiness consistency for ready ROB destinations.

## Random Testing

```bash
python tools/random_program_generator.py --instructions 100 --seed 42 --output generated/random_42.riscv
python tools/run_random_diff_tests.py --count 20 --instructions 100
```

Generated programs stay finite, keep memory accesses in a small valid range, and use simple RV32I/RV32M operations that the simulator already supports.
