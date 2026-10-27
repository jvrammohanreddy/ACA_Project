#include "dbp.h"
#include <algorithm>
#include <cassert>

// Initialize the static global predictor table
std::map<std::size_t, int> dbp::dead_block_predictor;

// Constructor routing
dbp::dbp(CACHE* cache) : dbp(cache, cache->NUM_SET, cache->NUM_WAY) {}

dbp::dbp(CACHE* cache, long sets, long ways) 
    : replacement(cache), NUM_WAY(ways), 
      last_used_cycles(static_cast<std::size_t>(sets * ways), 0),
      last_touch_pc(static_cast<std::size_t>(sets * ways), 0) {}

long dbp::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip, champsim::address full_addr, access_type type)
{
    auto begin = std::next(std::begin(last_used_cycles), set * NUM_WAY);
    auto end = std::next(begin, NUM_WAY);

    // 1. DBP LOGIC: Check for predicted dead blocks
    for (long way = 0; way < NUM_WAY; way++) {
        std::size_t block_pc = last_touch_pc.at(static_cast<std::size_t>(set * NUM_WAY + way));
        if (dead_block_predictor[block_pc] == 3) {
            return way; // Evict immediately!
        }
    }

    // 2. FALLBACK LOGIC: Standard LRU (copied safely from the default lru.cc)
    auto victim = std::min_element(begin, end);
    assert(begin <= victim);
    assert(victim < end);
    return std::distance(begin, victim);
}

void dbp::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr, access_type type)
{
    std::size_t index = static_cast<std::size_t>(set * NUM_WAY + way);
    
    // An insertion means the old block was evicted. 
    // Learn: Increment dead counter for the PC that brought the victim in.
    std::size_t evicted_pc = last_touch_pc.at(index);
    if (dead_block_predictor[evicted_pc] < 3) {
        dead_block_predictor[evicted_pc]++;
    }

    // Setup the new block
    last_used_cycles.at(index) = cycle++;
    last_touch_pc.at(index) = ip.to<std::size_t>();
}

void dbp::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr, access_type type, uint8_t hit)
{
    std::size_t index = static_cast<std::size_t>(set * NUM_WAY + way);
    
    if (hit && access_type{type} != access_type::WRITE) {
        last_used_cycles.at(index) = cycle++;
        
        // Learn: Block was reused, it is NOT dead. Decrement the counter for the PC that brought it in.
        std::size_t prev_pc = last_touch_pc.at(index);
        if (dead_block_predictor[prev_pc] > 0) {
            dead_block_predictor[prev_pc]--;
        }

        // Update the signature to the PC that just kept it alive
        last_touch_pc.at(index) = ip.to<std::size_t>();
    }
}