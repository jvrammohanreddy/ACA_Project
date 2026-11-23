#include "hint_rrip.h"
#include "../dbp/dbp.h"

hint_rrip::hint_rrip(CACHE* cache) : hint_rrip(cache, cache->NUM_SET, cache->NUM_WAY) {}

hint_rrip::hint_rrip(CACHE* cache, long sets, long ways) 
    : replacement(cache), NUM_WAY(ways), rrpv(static_cast<std::size_t>(sets * ways), 2) {}

long hint_rrip::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip, champsim::address full_addr, access_type type)
{
    std::size_t offset = static_cast<std::size_t>(set * NUM_WAY);
    
    // RRIP Logic: Find the first block with RRPV == 3
    while (true) {
        for (long way = 0; way < NUM_WAY; way++) {
            if (rrpv.at(offset + way) == MAX_RRPV) {
                return way; // Found the victim
            }
        }
        
        // Aging: If no block is at 3, increment all blocks in the set
        for (long way = 0; way < NUM_WAY; way++) {
            rrpv.at(offset + way)++;
        }
    }
    return 0;
}

void hint_rrip::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr, access_type type)
{
    std::size_t index = static_cast<std::size_t>(set * NUM_WAY + way);
    
    // --- THE CROSS-TIER HINT ---
    // We check the L1's global table for this incoming block's IP.
    // If the counter is 3, the L1 explicitly flagged this as a dead block.
    std::size_t incoming_pc = ip.to<std::size_t>();
    bool is_dead_hint = (dbp::dead_block_predictor[incoming_pc] == 3);

    if (is_dead_hint) {
        // HINT-AWARE INSERTION: Assign max RRPV so it is targeted for immediate eviction
        rrpv.at(index) = MAX_RRPV; 
    } else {
        // STANDARD INSERTION: Assign Long RRI (2)
        rrpv.at(index) = 2; 
    }
}

void hint_rrip::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr, access_type type, uint8_t hit)
{
    std::size_t index = static_cast<std::size_t>(set * NUM_WAY + way);
    
    // If the block is reused, it is highly valuable. Reset its interval to 0 (Near RRI).
    if (hit && access_type{type} != access_type::WRITE) {
        rrpv.at(index) = 0; 
    }
}