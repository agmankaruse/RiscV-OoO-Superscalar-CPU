#!/usr/bin/env python3
import argparse
import random
from pathlib import Path


def src_reg(rng):
    return f"x{rng.randint(1, 7)}"


def dest_reg(rng):
    return f"x{rng.randint(8, 15)}"


def generate(instructions, seed):
    rng = random.Random(seed)
    lines = [
        "# Generated finite RV32I/RV32M smoke program",
        "ADDI x1, x0, 1",
        "ADDI x2, x0, 2",
        "ADDI x3, x0, 3",
        "ADDI x4, x0, 4",
        "ADDI x5, x0, 5",
        "ADDI x6, x0, 6",
        "ADDI x7, x0, 7",
    ]

    op_count = max(0, instructions - len(lines) - 5)
    ops = ["ADD", "SUB", "XOR", "OR", "AND", "ADDI", "LW", "SW", "MUL"]
    for _ in range(op_count):
        op = rng.choice(ops)
        if op in {"ADD", "SUB", "XOR", "OR", "AND", "MUL"}:
            lines.append(f"{op} {dest_reg(rng)}, {src_reg(rng)}, {src_reg(rng)}")
        elif op == "ADDI":
            lines.append(f"ADDI {dest_reg(rng)}, {src_reg(rng)}, {rng.randint(-16, 16)}")
        elif op == "LW":
            address = rng.randrange(0, 64, 4)
            lines.append(f"LW {dest_reg(rng)}, {address}(x0)")
        elif op == "SW":
            address = rng.randrange(0, 64, 4)
            lines.append(f"SW {src_reg(rng)}, {address}(x0)")

    if instructions >= 16:
        lines.extend([
            "ADDI x29, x0, 0",
            "ADDI x30, x0, 3",
            "bounded_loop:",
            "ADDI x29, x29, 1",
            "BLT x29, x30, bounded_loop",
        ])

    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description="Generate finite simple RISC-V assembly programs.")
    parser.add_argument("--instructions", type=int, default=100)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generate(args.instructions, args.seed), encoding="utf-8")
    print(f"generated {output} seed={args.seed} instructions={args.instructions}")


if __name__ == "__main__":
    main()
