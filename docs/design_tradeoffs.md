# Design Tradeoffs

## Readable Timing Model

The simulator models cycle-level queues, functional-unit latency, cache stalls, and branch recovery without trying to reproduce every structure in a commercial core. This makes the code easier to inspect and test.

## In-Order Reference Model

The reference CPU intentionally ignores timing. Keeping it simple makes it a useful oracle for final architectural state.

## Sparse Memory

Memory is sparse and byte-addressable internally, with word-level operations used by the current ISA subset. This keeps examples compact while leaving room for narrower loads and stores later.

## Conservative LSQ

Loads wait when older store addresses are unknown and can forward from older ready stores. The model demonstrates memory ordering without a full memory dependence predictor.

## Local Tooling

The viewer has no external dependencies so traces can be inspected from a checked-out repository without a web build pipeline.
