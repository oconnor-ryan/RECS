#include "public.h"
#include <string.h>

/*
  Bitmask Section.

  A utility for creating, comparing, and setting bitmasks used by each entity to know what components and tags
  are attached to that entity.
*/

#define BYTE_INDEX(bit_index) ((bit_index) >> 3)


static void bitmask_clear(struct recs_comp_bitmask *mask, uint8_t value) {
  memset(mask->bytes, value ? 0xFF : 0, RECS_COMP_BITMASK_SIZE);
}

static void bitmask_set(struct recs_comp_bitmask *mask, uint64_t bit_index, uint8_t value) {
  uint64_t byte_index = BYTE_INDEX(bit_index); //divide by 8

  //index & 0b111 == index % 8
  uint8_t m = (uint8_t)1 << (bit_index & 7);
  if(value) {
    mask->bytes[byte_index] |= m;
  } else {
    mask->bytes[byte_index] &= ~m;
  }
}

static uint8_t bitmask_test(struct recs_comp_bitmask *mask, uint64_t bit_index) {
  // example
  // there are 60 bytes, we want the 100th bit
  // 60 bytes = 480 bits
  // 100 / 8 = 12 
  // 12th byte = 96 bit
  // To get the bit within the 12th byte, we take the 
  // modulus 8 (or AND with 7).
  // 100 & 7 == 0110 0100 & 0000 0111 == 0000 0100 == 4

  uint64_t byte_index = BYTE_INDEX(bit_index); //divide by 8
  uint8_t byte = mask->bytes[byte_index];

  //get bit within this byte using modulus OR AND 0b111
  return byte & ((uint8_t)1 << (bit_index & 7));
}

static void bitmask_and(struct recs_comp_bitmask *dest, struct recs_comp_bitmask *op1, struct recs_comp_bitmask *op2) {
  for(uint32_t i = 0; i < RECS_COMP_BITMASK_SIZE; i++) {
    dest->bytes[i] = op1->bytes[i] & op2->bytes[i];
  }
}

static uint8_t bitmask_eq(struct recs_comp_bitmask *op1, struct recs_comp_bitmask *op2) {
  //the last byte will NOT use the full 8 bits if the RECS_MAX_COMPONENTS + RECS_MAX_TAGS are not a multiple of 8.
  //Thus we will need to set the MSB bits of the last byte to a consistant value (ususally 0).
  for(uint32_t i = 0; i < RECS_COMP_BITMASK_SIZE - 1; i++) {
    if(op1->bytes[i] != op2->bytes[i]) {
      return 0;
    }
  }

  //RECS_COMP_BITMASK_NUM_BITS & 7 == 0, then all bits are being checked
  //RECS_COMP_BITMASK_NUM_BITS & 7 == 1, then all but 1 bit is being checked

  //get number of bits (starting from MSB) that we are setting to 0 so that we can check the equality of the LSB bits.
  const uint8_t num_unused_bits = RECS_COMP_BITMASK_NUM_BITS & 7;

  //grab the last byte and AND with (0xFF >> num_unused_bits) to set all unused MSB bits to 0
  uint8_t last_byte1 = op1->bytes[RECS_COMP_BITMASK_SIZE-1] & ((uint8_t)0xFF >> num_unused_bits);
  uint8_t last_byte2 = op2->bytes[RECS_COMP_BITMASK_SIZE-1] & ((uint8_t)0xFF >> num_unused_bits);

  return last_byte1 == last_byte2;
}
