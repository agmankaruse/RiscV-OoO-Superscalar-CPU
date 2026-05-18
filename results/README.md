# Results

`results/example_run/` contains generated outputs from the current simulator build. These files should be regenerated after behavior changes:

```bash
./build/riscv_ooo_sim examples/arithmetic_program.txt
./build/riscv_ooo_sim --trace examples/dependency_program.txt
./build/riscv_ooo_sim --diff examples/arithmetic_program.txt
./build/riscv_ooo_sim --timeline --timeline-csv outputs/cpu_pipeline_timeline.csv examples/dependency_program.txt
python tools/run_cpu_benchmarks.py
```

Do not hand-edit benchmark or timeline CSV results.

## Current Sample Outputs

Benchmark run:

```text
PASS branch_loop cycles=40 ipc=0.45
PASS dependency_chain cycles=21 ipc=0.38
PASS memory_stream cycles=41 ipc=0.22
```

Randomized diff run:

```text
random_diff passed=20 failed=0 count=20
```

The checked-in example results include timeline CSV files for `dependency_chain`, `branch_loop`, and `memory_stream`.
