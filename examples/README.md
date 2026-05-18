# Examples

Run examples from the repository root after building:

```bash
./build/riscv_ooo_sim examples/arithmetic_program.txt
./build/riscv_ooo_sim --trace examples/dependency_program.txt
./build/riscv_ooo_sim --diff examples/arithmetic_program.txt
./build/riscv_ooo_sim --timeline --timeline-csv outputs/cpu_pipeline_timeline.csv examples/dependency_program.txt
```

Representative outputs are generated under `results/example_run/`.

## Sample Output

Normal run:

```text
cycles=18 retired=6 ipc=0.33 cpi=3.00 branch_accuracy=0.00% branch_mispredicts=0 ...
architectural registers: x0=0 x1=5 x2=10 x3=15 x4=10 x5=9 x6=15
```

Diff run:

```text
PASS diff program=examples\arithmetic_program.txt cycles=18 ooo_final_pc=6 reference_final_pc=6 ooo_retired=6 reference_retired=6
```

Trace run:

```text
cycle 1
  I-cache miss at pc=0
cycle 2
  fetch stalled by I-cache miss
```

Timeline CSV:

```text
cycle,instruction_id,pc,instruction,stage,event,rob_index,iq_index,lsq_index,arch_dest,phys_dest,old_phys_dest,branch_prediction,cache_result
9,1,0,ADDI x1  x0  1,Instruction Fetch,FETCH,0,-1,-1,1,-1,-1,,I_HIT
```
