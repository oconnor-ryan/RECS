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



void bitmask_clear(uint8_t *mask, uint8_t value, uint32_t bitmask_size);

void bitmask_set(uint8_t *mask, uint64_t bit_index, uint8_t value);
uint8_t bitmask_test(uint8_t *mask, uint64_t bit_index);

void bitmask_and(uint8_t *dest, uint8_t *op1, uint8_t *op2, uint32_t bitmask_size);


#endif// BITMASK_H
