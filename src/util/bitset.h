#ifndef UTIL_BITSET_H
#define UTIL_BITSET_H

#include <stdint.h>
#include <stddef.h>



struct bitset {
  uint8_t *bytes;
  uint64_t num_bits;
};

struct bitset bitset_init(void *buffer, uint64_t num_bits);

//get value of bit at certain position.
// (0 for LSB, num_bits-1 for MSB)
uint8_t bitset_test(struct bitset *bits, uint64_t index);


//check if all bits are set
uint8_t bitset_test_all(struct bitset *bits);

//check if any bits are set
uint8_t bitset_test_any(struct bitset *bits);

//check if no bits are set
uint8_t bitset_test_none(struct bitset *bits);





//set all bits to a certain value
void bitset_set_all(struct bitset *bits, uint8_t value);



//set value of bit at certain position
// (0 for LSB, num_bits-1 for MSB)
void bitset_set(struct bitset *bits, uint64_t index, uint8_t value);


//bitwise operators
//note that you should be able to set op1 == res to allow in-place operations
void bitset_and(struct bitset *res, struct bitset *op1, struct bitset *op2);
void bitset_or(struct bitset *res, struct bitset *op1, struct bitset *op2);
void bitset_xor(struct bitset *res, struct bitset *op1, struct bitset *op2);
void bitset_not(struct bitset *res, struct bitset *op1);

//bitshift operations
//note that you should be able to set op1 == res to allow in-place operations
//returns the bit that is shifted out
//uint8_t bitset_shift_left(struct bitset *res, struct bitset *op1, uint64_t shift);
//uint8_t bitset_shift_right(struct bitset *res, struct bitset *op1, uint64_t shift);



#endif// UTIL_BITSET_H
