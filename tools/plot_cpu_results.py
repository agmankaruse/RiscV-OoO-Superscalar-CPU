#!/usr/bin/env python3
import argparse
import csv
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read_rows(path):
    with Path(path).open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def numeric(row, key):
    try:
        return float(str(row.get(key, "0")).replace("%", ""))
    except ValueError:
        return 0.0


def write_text_report(rows, output_dir):
    report = output_dir / "cpu_benchmark_report.txt"
    lines = ["CPU benchmark summary", ""]
    for row in rows:
        lines.append(
            f"{row['benchmark']}: cycles={row['cycles']} retired={row['retired_instructions']} "
            f"IPC={row['IPC']} branch_accuracy={row['branch_prediction_accuracy']} "
            f"cache_stalls={row['cache_miss_stall_cycles']}"
        )
    report.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"wrote {report}")


def main():
    parser = argparse.ArgumentParser(description="Generate CPU benchmark plots or text reports.")
    parser.add_argument("--input", default=str(ROOT / "outputs" / "cpu_benchmark_results.csv"))
    parser.add_argument("--output-dir", default=str(ROOT / "outputs" / "plots"))
    args = parser.parse_args()

    rows = read_rows(args.input)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    try:
        import matplotlib.pyplot as plt
    except Exception:
        write_text_report(rows, output_dir)
        return 0

    plots = [
        ("IPC", "IPC by benchmark", "cpu_ipc.png"),
        ("branch_prediction_accuracy", "Branch prediction accuracy", "cpu_branch_accuracy.png"),
        ("cache_miss_stall_cycles", "Cache miss stall cycles", "cpu_cache_stalls.png"),
        ("ROB_average_occupancy", "Average ROB occupancy", "cpu_rob_occupancy.png"),
        ("IQ_average_occupancy", "Average issue queue occupancy", "cpu_iq_occupancy.png"),
        ("LSQ_average_occupancy", "Average LSQ occupancy", "cpu_lsq_occupancy.png"),
    ]
    labels = [row["benchmark"] for row in rows]
    for key, title, filename in plots:
        plt.figure(figsize=(10, 4))
        plt.bar(labels, [numeric(row, key) for row in rows])
        plt.title(title)
        plt.xticks(rotation=30, ha="right")
        plt.tight_layout()
        path = output_dir / filename
        plt.savefig(path)
        plt.close()
        print(f"wrote {path}")

    write_text_report(rows, output_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
