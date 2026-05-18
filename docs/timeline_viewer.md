# Pipeline Timeline Viewer

Generate a timeline:

```bash
./build/riscv_ooo_sim --timeline --timeline-csv outputs/cpu_pipeline_timeline.csv examples/dependency_program.txt
```

Open `viewer/cpu_timeline_viewer.html` in a local browser and select the CSV file. The viewer displays instruction IDs down the side and cycles across the top. Events include fetch, decode, rename, dispatch, issue, execute start/done, writeback, commit, flush, and stall. Filters support instruction ID and stage.

The CSV columns are:

```text
cycle,instruction_id,pc,instruction,stage,event,rob_index,iq_index,lsq_index,arch_dest,phys_dest,old_phys_dest,branch_prediction,cache_result
```
