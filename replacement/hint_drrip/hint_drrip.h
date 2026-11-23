#ifndef HINT_DRRIP_H
#define HINT_DRRIP_H

#include <vector>
#include <cstddef>
#include "cache.h"
#include "modules.h"

class hint_drrip : public champsim::modules::replacement
{
  long NUM_WAY;
  std::vector<int> rrpv;
  const int MAX_RRPV = 3;

  // Set Dueling PSEL counter (10-bit: -512 to +511)
  // Positive = SRRIP is winning, Negative = BRRIP is winning
  int psel = 0;
  const int PSEL_MAX = 511;
  const int PSEL_MIN = -512;

  // BRRIP insertion probability (1/32 chance to insert at RRPV=2)
  const int BRRIP_PROBABILITY = 32;

public:
  explicit hint_drrip(CACHE* cache);
  hint_drrip(CACHE* cache, long sets, long ways);

  long find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip, champsim::address full_addr, access_type type);
  void replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr, access_type type);
  void update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr, access_type type, uint8_t hit);
};

#endif