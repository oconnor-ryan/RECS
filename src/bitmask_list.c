#include "bitmask_list.h"

void bitmask_list_init(struct bitmask_list *list, uint32_t bytes_per_mask, uint8_t *buffer) {
  list->bytes_per_mask = bytes_per_mask;
  list->buffer = buffer;
}

uint8_t* bitmask_list_get(struct bitmask_list *list, uint32_t index) {
  return (list->buffer + (index * list->bytes_per_mask));
}
