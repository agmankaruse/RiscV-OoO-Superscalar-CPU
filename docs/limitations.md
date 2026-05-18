# Limitations

- The ISA is a focused assembly subset, not a full RISC-V privileged or ABI environment.
- Memory examples currently use word loads and stores.
- Cache modeling is direct-mapped and latency-oriented, not a full hierarchy.
- Branch prediction is intentionally compact.
- The LSQ models conservative ordering and forwarding, but not memory dependence prediction.
- The simulator is deterministic and single-core.
- Random programs are finite smoke tests, not a complete formal generator.
- Benchmark results are useful for relative analysis inside this simulator, not claims about real hardware.
