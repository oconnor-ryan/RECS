#ifndef BITMASK_HANDLER
#define BITMASK_HANDLER

#include <stdint.h>

struct comp_bitmask_list {
  //holds
  uint8_t *buffer;
  uint64_t num_masks;
  uint64_t bits_per_mask;
  uint64_t bytes_per_mask;
};

void comp_bitmask_list_init(struct comp_bitmask_list *list, uint8_t *buffer, uint64_t num_masks, uint64_t num_bits);
void comp_bitmask_list_clear(struct comp_bitmask_list *list, uint8_t value);

void comp_bitmask_list_set_all_in_mask(struct comp_bitmask_list *list, uint64_t mask_index, uint8_t value);
void comp_bitmask_list_set(struct comp_bitmask_list *list, uint64_t mask_index, uint64_t bit_index, uint8_t value);

uint8_t comp_bitmask_list_test(struct comp_bitmask_list *list, uint64_t mask_index, uint64_t bit_index);




#endif// BITMASK_HANDLER
