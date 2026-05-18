#!/usr/bin/env python3
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def default_simulator():
    for directory in [ROOT / "build", ROOT / "build" / "Debug", ROOT / "build" / "Release"]:
        for name in ["riscv_ooo_sim.exe", "riscv_ooo_sim"]:
            candidate = directory / name
            if candidate.exists():
                return candidate
    return ROOT / "build" / "riscv_ooo_sim"


def parse_summary(output):
    fields = {}
    for token in output.replace("%", "").split():
        if "=" in token:
            key, value = token.split("=", 1)
            fields[key] = value
    return fields


def value(fields, name, default="0"):
    return fields.get(name, default)


def main():
    parser = argparse.ArgumentParser(description="Run CPU simulator benchmarks.")
    parser.add_argument("--sim", default=str(default_simulator()))
    parser.add_argument("--benchmarks", default=str(ROOT / "benchmarks"))
    parser.add_argument("--output", default=str(ROOT / "outputs" / "cpu_benchmark_results.csv"))
    args = parser.parse_args()

    benchmark_dir = Path(args.benchmarks)
    programs = sorted(list(benchmark_dir.glob("*.riscv")) + list(benchmark_dir.glob("*.txt")))
    if not programs:
        print(f"no benchmarks found in {benchmark_dir}", file=sys.stderr)
        return 1

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    timeline_dir = output_path.parent / "timelines"
    timeline_dir.mkdir(parents=True, exist_ok=True)

    rows = []
    for program in programs:
        timeline = timeline_dir / f"{program.stem}_timeline.csv"
        completed = subprocess.run(
            [args.sim, "--timeline", "--timeline-csv", str(timeline), str(program)],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        fields = parse_summary(completed.stdout)
        rows.append({
            "benchmark": program.stem,
            "status": "PASS" if completed.returncode == 0 else "FAIL",
            "cycles": value(fields, "cycles"),
            "retired_instructions": value(fields, "retired"),
            "IPC": value(fields, "ipc"),
            "branch_prediction_accuracy": value(fields, "branch_accuracy"),
            "ROB_average_occupancy": value(fields, "avg_rob"),
            "IQ_average_occupancy": value(fields, "avg_iq"),
            "LSQ_average_occupancy": value(fields, "avg_lsq"),
            "I_cache_hit_rate": value(fields, "icache_hit_rate"),
            "D_cache_hit_rate": value(fields, "dcache_hit_rate"),
            "flush_count": value(fields, "branch_mispredicts"),
            "stall_cycles": value(fields, "stalls_total", value(fields, "cache_miss_stalls")),
            "frontend_stall_cycles": value(fields, "frontend_stalls"),
            "backend_stall_cycles": value(fields, "backend_stalls"),
            "branch_mispredict_stall_cycles": value(fields, "branch_mispredict_stalls"),
            "cache_miss_stall_cycles": value(fields, "cache_miss_stalls"),
            "timeline_csv": str(timeline.relative_to(ROOT)),
        })
        print(f"{rows[-1]['status']} {program.stem} cycles={rows[-1]['cycles']} ipc={rows[-1]['IPC']}")

    with output_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    print(f"wrote {output_path}")
    return 0 if all(row["status"] == "PASS" for row in rows) else 1


if __name__ == "__main__":
    sys.exit(main())
