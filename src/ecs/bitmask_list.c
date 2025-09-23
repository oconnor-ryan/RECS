#include "ecs/bitmask_list.h"

void bitmask_list_init(struct bitmask_list *list, uint32_t bytes_per_mask, uint8_t *buffer) {
  list->bytes_per_mask = bytes_per_mask;
  list->buffer = buffer;
}

recs_comp_bitmask bitmask_list_get(struct bitmask_list *list, uint32_t index) {
  return (recs_comp_bitmask) (list->buffer + (index * list->bytes_per_mask));
}
