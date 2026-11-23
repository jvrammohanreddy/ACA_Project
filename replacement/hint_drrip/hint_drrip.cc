#include "hint_drrip.h"
#include "../dbp/dbp.h" // Cross-tier communication
#include <cstdlib>

hint_drrip::hint_drrip(CACHE* cache) : hint_drrip(cache, cache->NUM_SET, cache->NUM_WAY) {}

hint_drrip::hint_drrip(CACHE* cache, long sets, long ways) 
    : replacement(cache), NUM_WAY(ways), rrpv(static_cast<std::size_t>(sets * ways), 2) {}

long hint_drrip::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip, champsim::address full_addr, access_type type)
{
    std::size_t offset = static_cast<std::size_t>(set * NUM_WAY);
    
    while (true) {
        for (long way = 0; way < NUM_WAY; way++) {
            if (rrpv.at(offset + way) == MAX_RRPV) {
                return way; 
            }
        }
        
        // Age all blocks if no MAX_RRPV is found
        for (long way = 0; way < NUM_WAY; way++) {
            rrpv.at(offset + way)++;
        }
    }
    return 0;
}

void hint_drrip::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr, access_type type)
{
    std::size_t index = static_cast<std::size_t>(set * NUM_WAY + way);
    
    // --- 1. THE CROSS-TIER HINT (Absolute Override) ---
    std::size_t incoming_pc = ip.to<std::size_t>();
    if (dbp::dead_block_predictor[incoming_pc] == 3) {
        rrpv.at(index) = MAX_RRPV; // Instant death for L1 explicitly flagged blocks
        return; 
    }

    // --- 2. SET DUELING LOGIC (For standard blocks) ---
    // Use bitwise AND to designate 1 out of every 32 sets as a leader
    bool is_srrip_set = ((set & 0x1F) == 0); // Leader 0
    bool is_brrip_set = ((set & 0x1F) == 1); // Leader 1
    
    bool use_srrip = true;

    if (is_srrip_set) {
        use_srrip = true;
    } else if (is_brrip_set) {
        use_srrip = false;
    } else {
        // Follower Sets: Use PSEL confidence counter to decide the winner
        use_srrip = (psel >= 0);
    }

    // --- 3. INSERTION ---
    if (use_srrip) {
        rrpv.at(index) = 2; // SRRIP: Long RRI
    } else {
        // BRRIP: 1/32 chance of Long RRI, otherwise Distant RRI
        if ((std::rand() % BRRIP_PROBABILITY) == 0) {
            rrpv.at(index) = 2;
        } else {
            rrpv.at(index) = MAX_RRPV;
        }
    }
}

void hint_drrip::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr, access_type type, uint8_t hit)
{
    std::size_t index = static_cast<std::size_t>(set * NUM_WAY + way);
    
    if (hit && access_type{type} != access_type::WRITE) {
        rrpv.at(index) = 0; // Re-reference promotion (Near RRI)
        
        // Update PSEL counter if a leader set receives a hit
        bool is_srrip_set = ((set & 0x1F) == 0);
        bool is_brrip_set = ((set & 0x1F) == 1);
        
        if (is_srrip_set && psel < PSEL_MAX) {
            psel++; // SRRIP logic is successfully retaining reused blocks
        } else if (is_brrip_set && psel > PSEL_MIN) {
            psel--; // BRRIP logic is successfully retaining reused blocks
        }
    }
}