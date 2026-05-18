# CPU Architecture

The simulator uses an 8-stage macro-pipeline:

1. Fetch
2. Decode
3. Rename
4. Dispatch
5. Issue
6. Execute
7. Writeback
8. Commit

Fetch and decode are in-order. Rename maps architectural registers to physical registers and snapshots the rename table for control-flow recovery. Dispatch allocates ROB, issue queue, and LSQ entries. Issue selects ready operations for available functional units. Writeback marks physical registers ready. Commit retires ROB entries in program order and updates architectural mappings.

## Core Structures

- Rename table: current speculative mappings and committed architectural mappings
- Physical register file: speculative values and readiness bits
- ROB: in-order retirement, branch snapshots, store commit
- Issue queue: ready instruction selection
- LSQ: conservative load issue, store forwarding, in-order store memory update
- Branch predictor: next-PC prediction and mispredict accounting
- Caches: direct-mapped I-cache and D-cache latency model

The design favors readability and verification hooks over perfect hardware fidelity.
