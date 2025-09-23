#ifndef BITMASK_LIST_H
#define BITMASK_LIST_H

#include <stdint.h>
#include <stddef.h>
#include "recs.h"


struct bitmask_list {
  uint32_t bytes_per_mask;
  uint8_t *buffer;
};

void bitmask_list_init(struct bitmask_list *list, uint32_t bytes_per_mask, uint8_t *buffer);
recs_comp_bitmask bitmask_list_get(struct bitmask_list *list, uint32_t index);


#endif// BITMASK_LIST_H
