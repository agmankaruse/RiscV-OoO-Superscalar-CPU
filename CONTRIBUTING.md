# Contributing

Thanks for improving the simulator.

## Local Workflow

```bash
cmake -S . -B build -DENABLE_TESTS=ON -DENABLE_WARNINGS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Guidelines

- Keep changes focused and readable.
- Preserve existing examples and tests unless the behavior is intentionally changing.
- Add or update tests for simulator correctness changes.
- Update docs when CLI behavior, architecture behavior, or tooling changes.
- Prefer C++17 standard library facilities and avoid new external dependencies unless clearly justified.
