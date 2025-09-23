#ifndef BITMASK_H
#define BITMASK_H

#include <string.h>
#include "recs.h"
/*
  Bitmask Section.

  A utility for creating, comparing, and setting bitmasks used by each entity to know what components and tags
  are attached to that entity.
*/

#define BYTE_INDEX(bit_index) ((bit_index) >> 3)



void bitmask_clear(recs_comp_bitmask mask, uint8_t value, uint32_t bitmask_size);

void bitmask_set(recs_comp_bitmask mask, uint64_t bit_index, uint8_t value);
uint8_t bitmask_test(recs_comp_bitmask mask, uint64_t bit_index);

void bitmask_and(recs_comp_bitmask dest, recs_comp_bitmask op1, recs_comp_bitmask op2, uint32_t bitmask_size);

uint8_t bitmask_eq(recs_comp_bitmask op1, recs_comp_bitmask op2, uint64_t bitmask_num_bits);

#endif// BITMASK_H
