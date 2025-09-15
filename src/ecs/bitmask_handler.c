#include "bitmask_handler.h"
#include <string.h>
#define BYTE_INDEX(bit_index) ((bit_index) >> 3)


void comp_bitmask_list_init(struct comp_bitmask_list *list, uint8_t *buffer, uint64_t num_masks, uint64_t num_bits) {
  list->buffer = buffer;
  list->bits_per_mask = num_bits;
  list->num_masks = num_masks;
  list->bytes_per_mask = BYTE_INDEX(num_bits) + 1;
}

void comp_bitmask_list_clear(struct comp_bitmask_list *list, uint8_t value) {
  memset(list->buffer, value ? 0xFF : 0, list->bytes_per_mask * list->num_masks);
}

void comp_bitmask_list_set_all_in_mask(struct comp_bitmask_list *list, uint64_t mask_index, uint8_t value) {
  uint8_t *bitmask_start = list->buffer + (mask_index * list->bytes_per_mask);
  memset(bitmask_start, value ? 0xFF : 0, list->bytes_per_mask);
}

void comp_bitmask_list_set(struct comp_bitmask_list *list, uint64_t mask_index, uint64_t bit_index, uint8_t value) {
  uint8_t *mask_start = list->buffer + (list->bytes_per_mask * mask_index);

  uint64_t byte_index = BYTE_INDEX(bit_index); //divide by 8

  //index & 0b111 == index % 8
  uint8_t mask = (uint8_t)1 << (bit_index & 7);
  if(value) {
    mask_start[byte_index] |= mask;
  } else {
    mask_start[byte_index] &= ~mask;
  }
}

uint8_t comp_bitmask_list_test(struct comp_bitmask_list *list, uint64_t mask_index, uint64_t bit_index) {
  // example
  // there are 60 bytes, we want the 100th bit
  // 60 bytes = 480 bits
  // 100 / 8 = 12 
  // 12th byte = 96 bit
  // To get the bit within the 12th byte, we take the 
  // modulus 8 (or AND with 7).
  // 100 & 7 == 0110 0100 & 0000 0111 == 0000 0100 == 4

  uint8_t *mask_start = list->buffer + (list->bytes_per_mask * mask_index);
  uint64_t byte_index = BYTE_INDEX(bit_index); //divide by 8
  uint8_t byte = mask_start[byte_index];

  //get bit within this byte using modulus OR AND 0b111
  return byte & ((uint8_t)1 << (bit_index & 7));
}
