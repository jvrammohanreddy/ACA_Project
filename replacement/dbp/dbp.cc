#include "cache.h"
#include <map>
#include <vector>

// 1. Predictor Table: Maps an Instruction Pointer (PC) to a confidence counter (0 to 3)
// 3 = Highly likely to be dead, 0 = Highly likely to be reused
std::map<uint64_t, int> dead_block_predictor;

// 2. Parallel Cache Metadata: Tracks the last PC that accessed each block
std::vector<std::vector<uint64_t>> last_touch_pc;

// 3. Standard LRU counters for our fallback mechanism
std::vector<std::vector<uint64_t>> lru_counters;

void CACHE::initialize_replacement() {
    // Size the metadata arrays based on the specific cache's dimensions
    last_touch_pc.assign(NUM_SET, std::vector<uint64_t>(NUM_WAY, 0));
    lru_counters.assign(NUM_SET, std::vector<uint64_t>(NUM_WAY, 0));
}

void CACHE::update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit) {
    // Update the LRU fallback counter
    lru_counters[set][way] = current_cycle;

    if (hit) {
        // The block was reused! The previous PC was wrong if it predicted "Dead".
        uint64_t prev_pc = last_touch_pc[set][way];
        if (dead_block_predictor[prev_pc] > 0) {
            dead_block_predictor[prev_pc]--; // Decrement: Move towards "Alive"
        }
    } else {
        // A cache miss means the block previously in this slot was evicted.
        // Therefore, the last PC that touched it brought in a "Dead" block.
        uint64_t evicted_pc = last_touch_pc[set][way];
        if (dead_block_predictor[evicted_pc] < 3) {
            dead_block_predictor[evicted_pc]++; // Increment: Move towards "Dead"
        }
    }

    // Record the new PC (ip) that is currently accessing this cache block
    last_touch_pc[set][way] = ip;
}
uint32_t CACHE::find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type) {
    // 1. DBP LOGIC: Check if any block in this set is predicted dead
    for (uint32_t way = 0; way < NUM_WAY; way++) {
        uint64_t block_pc = last_touch_pc[set][way];
        if (dead_block_predictor[block_pc] == 3) {
            return way; // Evict the predicted dead block immediately
        }
    }

    // 2. FALLBACK LOGIC: If no block is predicted dead, use standard LRU
    uint32_t lru_way = 0;
    uint64_t oldest_cycle = lru_counters[set][0];
    
    for (uint32_t way = 1; way < NUM_WAY; way++) {
        if (lru_counters[set][way] < oldest_cycle) {
            oldest_cycle = lru_counters[set][way];
            lru_way = way;
        }
    }
    
    return lru_way;
}

// Required by ChampSim to compile, even if empty
void CACHE::replacement_final_stats() {}