#include "bitset.h"
#include <string.h>

#include "../def.h"

#define BYTE_INDEX(bit_index) bit_index >> 3

struct bitset bitset_init(void *buffer, uint64_t num_bits) {
  struct bitset bits;
  bits.bytes = buffer;
  bits.num_bits = num_bits;
  return bits;
}

//get value of bit at certain position.
// (0 for LSB, num_bits-1 for MSB)
uint8_t bitset_test(struct bitset *bits, uint64_t index) {
  // example
  // there are 60 bytes, we want the 100th bit
  // 60 bytes = 480 bits
  // 100 / 8 = 12 
  // 12th byte = 96 bit
  // To get the bit within the 12th byte, we take the 
  // modulus 8 (or AND with 7).
  // 100 & 7 == 0110 0100 & 0000 0111 == 0000 0100 == 4

  uint64_t byte_index = BYTE_INDEX(index); //divide by 8
  uint8_t byte = bits->bytes[byte_index];

  //get bit within this byte using modulus OR AND 0b111
  return byte & ((uint8_t)1 << (index & 7));
}


//check if all bits are set
uint8_t bitset_test_all(struct bitset *bits) {
  uint64_t num_bytes = BYTE_INDEX(bits->num_bits);

  for(uint64_t i = 0; i < num_bytes; i++) {
    if(bits->bytes[i] == 0) {
      return 0;
    }
  }
  return 1;
}

//check if any bits are set
uint8_t bitset_test_any(struct bitset *bits) {
  uint64_t num_bytes = BYTE_INDEX(bits->num_bits);

  for(uint64_t i = 0; i < num_bytes; i++) {
    if(bits->bytes[i]) {
      return 1;
    }
  }
  return 0;
}

//check if no bits are set
uint8_t bitset_test_none(struct bitset *bits) {
  uint64_t num_bytes = BYTE_INDEX(bits->num_bits);

  for(uint64_t i = 0; i < num_bytes; i++) {
    if(bits->bytes[i]) {
      return 0;
    }
  }
  return 1;
}




//set all bits to a certain value
void bitset_set_all(struct bitset *bits, uint8_t value) {
  value = value ? 0xFF : 0;
  memset(bits->bytes, value, BYTE_INDEX(bits->num_bits));
}



//set value of bit at certain position
// (0 for LSB, num_bits-1 for MSB)
void bitset_set(struct bitset *bits, uint64_t index, uint8_t value) {
  uint64_t byte_index = BYTE_INDEX(index); //divide by 8

  //index & 0b111 == index % 8
  uint8_t mask = (uint8_t)1 << (index & 7);
  if(value) {
    bits->bytes[byte_index] |= mask;
  } else {
    bits->bytes[byte_index] &= ~mask;
  }
}



//bitwise operators
//note that you should be able to set op1 == res to allow in-place operations
void bitset_and(struct bitset *res, struct bitset *op1, struct bitset *op2) {
  ASSERT(res->num_bits == op1->num_bits && res->num_bits == op2->num_bits);
  uint64_t num_bytes = BYTE_INDEX(res->num_bits);

  for(uint64_t i = 0; i < num_bytes; i++) {
    res->bytes[i] = op1->bytes[i] & op2->bytes[i];
  }
}

void bitset_or(struct bitset *res, struct bitset *op1, struct bitset *op2) {
  ASSERT(res->num_bits == op1->num_bits && res->num_bits == op2->num_bits);
  uint64_t num_bytes = BYTE_INDEX(res->num_bits);

  for(uint64_t i = 0; i < num_bytes; i++) {
    res->bytes[i] = op1->bytes[i] | op2->bytes[i];
  }
}

void bitset_xor(struct bitset *res, struct bitset *op1, struct bitset *op2) {
  ASSERT(res->num_bits == op1->num_bits && res->num_bits == op2->num_bits);
  uint64_t num_bytes = BYTE_INDEX(res->num_bits);

  for(uint64_t i = 0; i < num_bytes; i++) {
    res->bytes[i] = op1->bytes[i] ^ op2->bytes[i];
  }
}

void bitset_not(struct bitset *res, struct bitset *op1) {
  ASSERT(res->num_bits == op1->num_bits);
  uint64_t num_bytes = BYTE_INDEX(res->num_bits);

  for(uint64_t i = 0; i < num_bytes; i++) {
    res->bytes[i] = ~op1->bytes[i];
  }
}


