#include "ecs/bitmask.h"

#define BYTE_INDEX(bit_index) ((bit_index) >> 3)



void bitmask_clear(uint8_t *mask, uint8_t value, uint32_t bitmask_size) {
  memset(mask, value ? 0xFF : 0, bitmask_size);
}

void bitmask_set(uint8_t *mask, uint64_t bit_index, uint8_t value) {
  uint64_t byte_index = BYTE_INDEX(bit_index); //divide by 8

  //index & 0b111 == index % 8
  uint8_t m = (uint8_t)1 << (bit_index & 7);
  if(value) {
    mask[byte_index] |= m;
  } else {
    mask[byte_index] &= ~m;
  }
}

uint8_t bitmask_test(uint8_t *mask, uint64_t bit_index) {
  // example
  // there are 60 bytes, we want the 100th bit
  // 60 bytes = 480 bits
  // 100 / 8 = 12 
  // 12th byte = 96 bit
  // To get the bit within the 12th byte, we take the 
  // modulus 8 (or AND with 7).
  // 100 & 7 == 0110 0100 & 0000 0111 == 0000 0100 == 4

  uint64_t byte_index = BYTE_INDEX(bit_index); //divide by 8
  uint8_t byte = mask[byte_index];

  //get bit within this byte using modulus OR AND 0b111
  return byte & ((uint8_t)1 << (bit_index & 7));
}

void bitmask_and(uint8_t *dest, uint8_t *op1, uint8_t *op2, uint32_t bitmask_size) {
  for(uint32_t i = 0; i < bitmask_size; i++) {
    dest[i] = op1[i] & op2[i];
  }
}

uint8_t bitmask_eq(uint8_t *op1, uint8_t *op2, uint64_t bitmask_num_bits) {
  uint32_t bitmask_size = BYTE_INDEX(bitmask_num_bits);

  //the last byte will NOT use the full 8 bits if the RECS_MAX_COMPONENTS + RECS_MAX_TAGS are not a multiple of 8.
  //Thus we will need to set the MSB bits of the last byte to a consistant value (ususally 0).
  for(uint32_t i = 0; i < bitmask_size - 1; i++) {
    if(op1[i] != op2[i]) {
      return 0;
    }
  }

  //RECS_COMP_BITMASK_NUM_BITS & 7 == 0, then all bits are being checked
  //RECS_COMP_BITMASK_NUM_BITS & 7 == 1, then all but 1 bit is being checked

  //get number of bits (starting from MSB) that we are setting to 0 so that we can check the equality of the LSB bits.
  const uint8_t num_unused_bits = bitmask_num_bits & 7;

  //grab the last byte and AND with (0xFF >> num_unused_bits) to set all unused MSB bits to 0
  uint8_t last_byte1 = op1[bitmask_size-1] & ((uint8_t)0xFF >> num_unused_bits);
  uint8_t last_byte2 = op2[bitmask_size-1] & ((uint8_t)0xFF >> num_unused_bits);

  return (last_byte1 & last_byte2) == last_byte2;
}


