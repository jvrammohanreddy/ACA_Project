# Cross-Tier Dead Block Management 

**Author:** Venkata Ram Mohan Reddy Jalireddygari (Roll No: 26M0770)  
**Course:** Advanced Computer Architecture

This repository contains a custom multi-tier cache replacement architecture implemented within the **ChampSim** simulator. The project aims to eliminate Last-Level Cache (LLC) pollution by mathematically identifying "dead blocks" in the L1 Data Cache (L1D) and passing a 1-bit eviction hint down the interconnect to guide L2 cache replacement policies.

## The Problem: Cache Pollution
In standard cache hierarchies, tiers operate in isolation. When the L1D evicts a block, it drops it down to the L2 cache. Lacking contextual history, the L2 blindly assumes this arriving data is highly valuable and inserts it at the Most Recently Used (MRU) position. If the CPU is actually finished with this data (a "dead block"), treating it as high-priority wastes massive amounts of L2 capacity and forces out genuinely useful data.

## Proposed Architecture
This project establishes a low-overhead communication channel between cache tiers using three core mechanisms:

1. **Local Prediction (L1D):** A PC-based signature table tracks the Instruction Pointer (PC) that fetched each cache block. Using a 2-bit saturating counter, the L1D learns if a specific instruction tends to fetch highly reusable or "one-time-use" data. When evicted, blocks with maxed-out counters are flagged as dead.
2. **The Metadata Hint (Interconnect):** Instead of dropping the block silently, the L1D attaches a 1-bit "dead hint" to the eviction packet sent down the memory bus, piggybacking on existing traffic with near-zero overhead.
3. **Hint-Aware Insertion (L2C):** A custom Re-Reference Interval Prediction (RRIP) policy in the L2 reads the incoming 1-bit hint. If flagged as dead, the L2 overrides standard insertion and assigns the block the maximum RRPV (Re-Reference Prediction Value). This targets the dead block for immediate eviction, preventing L2 pollution.

## Project Structure
Modifications are built on top of the modern ChampSim replacement API:
* `replacement/dbp/dbp.h` & `dbp.cc`: The L1D Dead Block Predictor utilizing a global `std::map` of PC signatures and 2-bit counters.
* `replacement/hint_lru/hint_lru.cc`: *(In Progress)* The hint-aware L2 insertion policy.
* `scripts/run_baseline.sh`: Automation script for benchmarking DPC-3 traces.

##  Checkpoint 1 Results
Initial phase testing validates the L1D predictor (`dbp.cc`) against a standard LRU baseline. Simulations were executed using the memory-intensive `429.mcf-22B` DPC-3 trace with a 10M instruction warmup and 50M instruction simulation window.
can find the results in results folder

## 🚀 How to Run

1. **Configure and Build ChampSim:**
   Compile the simulator with the custom L1D predictor and standard L2 LRU (or the future hint-aware L2 policy).
   ```bash
   ./config.sh champsim_config.json
   make
   ```

2. **Run the Simulation Script:**
   Ensure your benchmarking scripts are executable and run them against your downloaded DPC-3 traces.
   ```bash
   chmod +x run_baseline.sh
   ./run_baseline.sh
   ```
